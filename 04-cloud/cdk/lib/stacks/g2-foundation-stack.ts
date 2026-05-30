import * as cdk from 'aws-cdk-lib';
import * as iam from 'aws-cdk-lib/aws-iam';
import * as iot from 'aws-cdk-lib/aws-iot';
import * as kms from 'aws-cdk-lib/aws-kms';
import * as ssm from 'aws-cdk-lib/aws-ssm';
import { Construct } from 'constructs';
import type { G2Env } from '../config';
import { buildG2IotThingPolicyDocument } from '../constructs/g2-iot-thing-policy-document';
import { hyphenName } from '../naming';
import { ssmConfigPath, ssmFoundationPath } from '../config/ssm-paths';

export interface G2FoundationStackProps extends cdk.StackProps {
  readonly g2Env: G2Env;
}

/**
 * M1 — KMS, SSM, shared Lambda role, G2 IoT Thing policy template.
 */
export class G2FoundationStack extends cdk.Stack {
  public readonly kmsKey: kms.Key;
  public readonly lambdaBaseRole: iam.Role;
  public readonly iotThingPolicy: iot.CfnPolicy;

  constructor(scope: Construct, id: string, props: G2FoundationStackProps) {
    super(scope, id, props);

    const { g2Env } = props;
    const region = cdk.Stack.of(this).region;
    const account = cdk.Stack.of(this).account;

    cdk.Tags.of(this).add('Project', 'IQEdge-G2');
    cdk.Tags.of(this).add('Environment', g2Env);
    cdk.Tags.of(this).add('ManagedBy', 'cdk');
    cdk.Tags.of(this).add('Stack', 'foundation');

    this.kmsKey = new kms.Key(this, 'G2CMK', {
      alias: `alias/${hyphenName(g2Env, 'cmk')}`,
      description: `IQEdge G2 ${g2Env} — Timestream/S3 encryption`,
      enableKeyRotation: true,
      removalPolicy:
        g2Env === 'prod' ? cdk.RemovalPolicy.RETAIN : cdk.RemovalPolicy.DESTROY,
    });

    this.lambdaBaseRole = new iam.Role(this, 'LambdaBaseRole', {
      roleName: hyphenName(g2Env, 'role-lambda-base'),
      description: `G2 ${g2Env} shared Lambda execution role (ingest/API)`,
      assumedBy: new iam.ServicePrincipal('lambda.amazonaws.com'),
      managedPolicies: [
        iam.ManagedPolicy.fromAwsManagedPolicyName(
          'service-role/AWSLambdaBasicExecutionRole',
        ),
      ],
    });

    // M2/M4 stacks will attach domain-scoped inline policies to this role or per-function roles.
    this.lambdaBaseRole.addToPolicy(
      new iam.PolicyStatement({
        sid: 'G2SSMReadFoundation',
        actions: ['ssm:GetParameter', 'ssm:GetParameters'],
        resources: [
          `arn:aws:ssm:${region}:${account}:parameter/iqedge/g2/${g2Env}/*`,
        ],
      }),
    );

    const policyName = hyphenName(g2Env, 'iot-policy-g2-device');
    this.iotThingPolicy = new iot.CfnPolicy(this, 'G2DeviceIotPolicy', {
      policyName,
      policyDocument: buildG2IotThingPolicyDocument({ g2Env, region, account }),
    });

    new ssm.StringParameter(this, 'ParamKmsKeyArn', {
      parameterName: ssmFoundationPath(g2Env, 'kms-key-arn'),
      stringValue: this.kmsKey.keyArn,
      description: `G2 ${g2Env} KMS CMK ARN`,
    });

    new ssm.StringParameter(this, 'ParamLambdaBaseRoleArn', {
      parameterName: ssmFoundationPath(g2Env, 'lambda-base-role-arn'),
      stringValue: this.lambdaBaseRole.roleArn,
      description: `G2 ${g2Env} Lambda base role ARN`,
    });

    new ssm.StringParameter(this, 'ParamIotPolicyName', {
      parameterName: ssmFoundationPath(g2Env, 'iot-thing-policy-name'),
      stringValue: policyName,
      description: `G2 ${g2Env} IoT policy name for Things`,
    });

    new ssm.StringParameter(this, 'ParamG2Env', {
      parameterName: ssmConfigPath(g2Env, 'g2-env'),
      stringValue: g2Env,
      description: `Active G2 environment tag (${g2Env})`,
    });

    new cdk.CfnOutput(this, 'G2Env', { value: g2Env });
    new cdk.CfnOutput(this, 'KmsKeyArn', { value: this.kmsKey.keyArn });
    new cdk.CfnOutput(this, 'LambdaBaseRoleArn', {
      value: this.lambdaBaseRole.roleArn,
    });
    new cdk.CfnOutput(this, 'IotThingPolicyName', { value: policyName });
    new cdk.CfnOutput(this, 'SsmPrefix', {
      value: `/iqedge/g2/${g2Env}/`,
    });
  }
}
