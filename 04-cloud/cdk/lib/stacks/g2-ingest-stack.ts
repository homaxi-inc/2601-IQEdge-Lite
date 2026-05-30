import * as cdk from 'aws-cdk-lib';
import * as iam from 'aws-cdk-lib/aws-iam';
import * as iot from 'aws-cdk-lib/aws-iot';
import * as lambda from 'aws-cdk-lib/aws-lambda';
import * as logs from 'aws-cdk-lib/aws-logs';
import * as ssm from 'aws-cdk-lib/aws-ssm';
import { Construct } from 'constructs';
import * as path from 'path';
import type { G2Env } from '../config';
import { hyphenName, iotRuleName, mqttTopicPrefix } from '../naming';
import {
  ssmFoundationPath,
  ssmIngestPath,
  ssmRegistryPath,
  ssmStoragePath,
} from '../config/ssm-paths';

export interface G2IngestStackProps extends cdk.StackProps {
  readonly g2Env: G2Env;
}

/**
 * M4 — energy domain IoT Rule + ingest Lambda (Timestream + Shadow).
 */
export class G2IngestStack extends cdk.Stack {
  public readonly ingestEnergyFn: lambda.Function;
  public readonly energyRule: iot.CfnTopicRule;

  constructor(scope: Construct, id: string, props: G2IngestStackProps) {
    super(scope, id, props);

    const { g2Env } = props;
    const region = cdk.Stack.of(this).region;
    const account = cdk.Stack.of(this).account;

    cdk.Tags.of(this).add('Project', 'IQEdge-G2');
    cdk.Tags.of(this).add('Environment', g2Env);
    cdk.Tags.of(this).add('ManagedBy', 'cdk');
    cdk.Tags.of(this).add('Stack', 'ingest');

    const timestreamDb = ssm.StringParameter.valueForStringParameter(
      this,
      ssmStoragePath(g2Env, 'timestream-database'),
    );
    const timestreamTable = ssm.StringParameter.valueForStringParameter(
      this,
      ssmStoragePath(g2Env, 'timestream-table-energy'),
    );
    const shadowTable = ssm.StringParameter.valueForStringParameter(
      this,
      ssmStoragePath(g2Env, 'dynamodb-shadow-table'),
    );
    const registryTable = ssm.StringParameter.valueForStringParameter(
      this,
      ssmRegistryPath(g2Env, 'dynamodb-registry-table'),
    );

    const fnName = hyphenName(g2Env, 'fn-ingest-energy');
    const ruleName = iotRuleName(g2Env, 'rule', 'energy');
    const topic = mqttTopicPrefix(g2Env, 'energy', 'telemetry');

    this.ingestEnergyFn = new lambda.Function(this, 'IngestEnergyFn', {
      functionName: fnName,
      description: `G2 ${g2Env} energy telemetry ingest`,
      runtime: lambda.Runtime.PYTHON_3_12,
      handler: 'handler.lambda_handler',
      code: lambda.Code.fromAsset(
        path.join(__dirname, '..', '..', 'lambda', 'ingest-energy'),
      ),
      timeout: cdk.Duration.seconds(30),
      memorySize: 256,
      logRetention: logs.RetentionDays.TWO_WEEKS,
      environment: {
        G2_ENV: g2Env,
        TIMESTREAM_DATABASE: timestreamDb,
        TIMESTREAM_TABLE: timestreamTable,
        SHADOW_TABLE: shadowTable,
        REGISTRY_TABLE: registryTable,
        METRIC_NAMESPACE: `IQEdge/G2/${g2Env}/Ingest`,
      },
    });

    const tsTableArn = `arn:aws:timestream:${region}:${account}:database/${timestreamDb}/table/${timestreamTable}`;
    const tsDbArn = `arn:aws:timestream:${region}:${account}:database/${timestreamDb}`;

    this.ingestEnergyFn.addToRolePolicy(
      new iam.PolicyStatement({
        sid: 'TimestreamWriteEnergy',
        actions: ['timestream:WriteRecords', 'timestream:DescribeEndpoints'],
        resources: [tsTableArn, tsDbArn],
      }),
    );
    this.ingestEnergyFn.addToRolePolicy(
      new iam.PolicyStatement({
        sid: 'TimestreamDescribeEndpoints',
        actions: ['timestream:DescribeEndpoints'],
        resources: ['*'],
      }),
    );

    this.ingestEnergyFn.addToRolePolicy(
      new iam.PolicyStatement({
        sid: 'DdbShadowWrite',
        actions: ['dynamodb:PutItem', 'dynamodb:UpdateItem'],
        resources: [
          `arn:aws:dynamodb:${region}:${account}:table/${shadowTable}`,
        ],
      }),
    );

    this.ingestEnergyFn.addToRolePolicy(
      new iam.PolicyStatement({
        sid: 'DdbRegistryReadWriteFirmware',
        actions: ['dynamodb:GetItem', 'dynamodb:UpdateItem'],
        resources: [
          `arn:aws:dynamodb:${region}:${account}:table/${registryTable}`,
        ],
      }),
    );

    this.ingestEnergyFn.addToRolePolicy(
      new iam.PolicyStatement({
        sid: 'CloudWatchMetrics',
        actions: ['cloudwatch:PutMetricData'],
        resources: ['*'],
      }),
    );

    this.energyRule = new iot.CfnTopicRule(this, 'EnergyTelemetryRule', {
      ruleName,
      topicRulePayload: {
        sql: `SELECT * FROM '${topic}'`,
        awsIotSqlVersion: '2016-03-23',
        actions: [
          {
            lambda: {
              functionArn: this.ingestEnergyFn.functionArn,
            },
          },
        ],
        errorAction: {
          cloudwatchLogs: {
            logGroupName: `/aws/iot/rules/${ruleName}`,
            roleArn: this.createRuleErrorRole(ruleName).roleArn,
          },
        },
      },
    });

    new lambda.CfnPermission(this, 'IoTEnergyRuleInvoke', {
      action: 'lambda:InvokeFunction',
      functionName: this.ingestEnergyFn.functionName,
      principal: 'iot.amazonaws.com',
      sourceArn: `arn:aws:iot:${region}:${account}:rule/${ruleName}`,
    });

    new ssm.StringParameter(this, 'ParamIngestEnergyFnArn', {
      parameterName: ssmIngestPath(g2Env, 'lambda-ingest-energy-arn'),
      stringValue: this.ingestEnergyFn.functionArn,
    });

    new ssm.StringParameter(this, 'ParamRuleEnergyName', {
      parameterName: ssmIngestPath(g2Env, 'iot-rule-energy-name'),
      stringValue: ruleName,
    });

    new ssm.StringParameter(this, 'ParamEnergyMqttTopic', {
      parameterName: ssmIngestPath(g2Env, 'mqtt-topic-energy-telemetry'),
      stringValue: topic,
    });

    new cdk.CfnOutput(this, 'IngestEnergyFnArn', {
      value: this.ingestEnergyFn.functionArn,
    });
    new cdk.CfnOutput(this, 'EnergyRuleName', { value: ruleName });
    new cdk.CfnOutput(this, 'EnergyMqttTopic', { value: topic });
  }

  private createRuleErrorRole(ruleName: string): iam.Role {
    const logGroup = new logs.LogGroup(this, 'RuleErrorLogGroup', {
      logGroupName: `/aws/iot/rules/${ruleName}`,
      retention: logs.RetentionDays.TWO_WEEKS,
      removalPolicy: cdk.RemovalPolicy.DESTROY,
    });

    const role = new iam.Role(this, 'RuleErrorRole', {
      assumedBy: new iam.ServicePrincipal('iot.amazonaws.com'),
    });
    logGroup.grantWrite(role);
    return role;
  }
}
