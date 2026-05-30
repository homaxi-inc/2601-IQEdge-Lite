import * as cdk from 'aws-cdk-lib';
import * as dynamodb from 'aws-cdk-lib/aws-dynamodb';
import * as iam from 'aws-cdk-lib/aws-iam';
import * as ssm from 'aws-cdk-lib/aws-ssm';
import { Construct } from 'constructs';
import type { G2Env } from '../config';
import { hyphenName } from '../naming';
import { ssmFoundationPath, ssmRegistryPath } from '../config/ssm-paths';

export interface G2RegistryStackProps extends cdk.StackProps {
  readonly g2Env: G2Env;
}

/**
 * M3 — System Registry (Fleet primary key sys_id) + alias GSI for Legacy 合流.
 */
export class G2RegistryStack extends cdk.Stack {
  public readonly registryTable: dynamodb.Table;

  constructor(scope: Construct, id: string, props: G2RegistryStackProps) {
    super(scope, id, props);

    const { g2Env } = props;

    cdk.Tags.of(this).add('Project', 'IQEdge-G2');
    cdk.Tags.of(this).add('Environment', g2Env);
    cdk.Tags.of(this).add('ManagedBy', 'cdk');
    cdk.Tags.of(this).add('Stack', 'registry');

    this.registryTable = new dynamodb.Table(this, 'RegistryTable', {
      tableName: hyphenName(g2Env, 'table-registry'),
      partitionKey: { name: 'sys_id', type: dynamodb.AttributeType.STRING },
      billingMode: dynamodb.BillingMode.PAY_PER_REQUEST,
      pointInTimeRecovery: g2Env === 'prod',
      removalPolicy:
        g2Env === 'prod' ? cdk.RemovalPolicy.RETAIN : cdk.RemovalPolicy.DESTROY,
      encryption: dynamodb.TableEncryption.AWS_MANAGED,
    });

    this.registryTable.addGlobalSecondaryIndex({
      indexName: 'gsi-alias-mppt',
      partitionKey: {
        name: 'alias_mppt_serial',
        type: dynamodb.AttributeType.STRING,
      },
      projectionType: dynamodb.ProjectionType.ALL,
    });

    this.registryTable.addGlobalSecondaryIndex({
      indexName: 'gsi-alias-legacy-device',
      partitionKey: {
        name: 'alias_legacy_device_id',
        type: dynamodb.AttributeType.STRING,
      },
      projectionType: dynamodb.ProjectionType.ALL,
    });

    const lambdaRoleArn = ssm.StringParameter.valueForStringParameter(
      this,
      ssmFoundationPath(g2Env, 'lambda-base-role-arn'),
    );
    const lambdaBaseRole = iam.Role.fromRoleArn(
      this,
      'ImportedLambdaBaseRole',
      lambdaRoleArn,
    );
    this.registryTable.grantReadData(lambdaBaseRole);

    new ssm.StringParameter(this, 'ParamRegistryTableName', {
      parameterName: ssmRegistryPath(g2Env, 'dynamodb-registry-table'),
      stringValue: this.registryTable.tableName,
    });

    new ssm.StringParameter(this, 'ParamRegistryGsiMppt', {
      parameterName: ssmRegistryPath(g2Env, 'dynamodb-registry-gsi-mppt'),
      stringValue: 'gsi-alias-mppt',
    });

    new cdk.CfnOutput(this, 'RegistryTableName', {
      value: this.registryTable.tableName,
    });
  }
}
