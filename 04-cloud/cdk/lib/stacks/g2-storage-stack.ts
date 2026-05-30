import * as cdk from 'aws-cdk-lib';
import * as dynamodb from 'aws-cdk-lib/aws-dynamodb';
import * as kms from 'aws-cdk-lib/aws-kms';
import * as s3 from 'aws-cdk-lib/aws-s3';
import * as ssm from 'aws-cdk-lib/aws-ssm';
import * as timestream from 'aws-cdk-lib/aws-timestream';
import { Construct } from 'constructs';
import type { G2Env } from '../config';
import { G2_DOMAINS } from '../config/domains';
import { G2_TIMESTREAM_TABLE_SUFFIX } from '../config/timestream-tables';
import { G2TimestreamDomainTable } from '../constructs/g2-timestream-domain-table';
import { hyphenName, underscoreName } from '../naming';
import { ssmFoundationPath, ssmStoragePath } from '../config/ssm-paths';

export interface G2StorageStackProps extends cdk.StackProps {
  readonly g2Env: G2Env;
}

/**
 * M2 — Timestream (5 tables), DDB Shadow, S3 vision assets, DDB control audit.
 */
export class G2StorageStack extends cdk.Stack {
  public readonly database: timestream.CfnDatabase;
  public readonly shadowTable: dynamodb.Table;
  public readonly controlLogsTable: dynamodb.Table;
  public readonly visionBucket: s3.Bucket;
  public readonly domainTables: Record<string, timestream.CfnTable> = {};

  constructor(scope: Construct, id: string, props: G2StorageStackProps) {
    super(scope, id, props);

    const { g2Env } = props;
    const dbName = underscoreName(g2Env, 'database');

    cdk.Tags.of(this).add('Project', 'IQEdge-G2');
    cdk.Tags.of(this).add('Environment', g2Env);
    cdk.Tags.of(this).add('ManagedBy', 'cdk');
    cdk.Tags.of(this).add('Stack', 'storage');

    const kmsKeyArn = ssm.StringParameter.valueForStringParameter(
      this,
      ssmFoundationPath(g2Env, 'kms-key-arn'),
    );
    const kmsKey = kms.Key.fromKeyArn(this, 'ImportedKmsKey', kmsKeyArn);

    this.database = new timestream.CfnDatabase(this, 'G2Database', {
      databaseName: dbName,
    });

    let previousTable: timestream.CfnTable | undefined;
    for (const domain of G2_DOMAINS) {
      const tableName = underscoreName(g2Env, G2_TIMESTREAM_TABLE_SUFFIX[domain]);
      const construct = new G2TimestreamDomainTable(this, `TsTable${domain}`, {
        databaseName: dbName,
        tableName,
        domain,
        memoryRetentionHours: g2Env === 'prod' ? 48 : 24,
        magneticRetentionDays: g2Env === 'prod' ? 730 : 365,
      });
      construct.table.addDependency(this.database);
      if (previousTable) {
        construct.table.addDependency(previousTable);
      }
      previousTable = construct.table;
      this.domainTables[domain] = construct.table;
    }

    this.shadowTable = new dynamodb.Table(this, 'ShadowTable', {
      tableName: hyphenName(g2Env, 'table-shadow'),
      partitionKey: { name: 'pk', type: dynamodb.AttributeType.STRING },
      sortKey: { name: 'sk', type: dynamodb.AttributeType.STRING },
      billingMode: dynamodb.BillingMode.PAY_PER_REQUEST,
      pointInTimeRecovery: g2Env === 'prod',
      removalPolicy:
        g2Env === 'prod' ? cdk.RemovalPolicy.RETAIN : cdk.RemovalPolicy.DESTROY,
      encryption: dynamodb.TableEncryption.AWS_MANAGED,
    });

    this.controlLogsTable = new dynamodb.Table(this, 'ControlLogsTable', {
      tableName: hyphenName(g2Env, 'table-control-logs'),
      partitionKey: { name: 'pk', type: dynamodb.AttributeType.STRING },
      sortKey: { name: 'sk', type: dynamodb.AttributeType.STRING },
      billingMode: dynamodb.BillingMode.PAY_PER_REQUEST,
      pointInTimeRecovery: g2Env === 'prod',
      removalPolicy:
        g2Env === 'prod' ? cdk.RemovalPolicy.RETAIN : cdk.RemovalPolicy.DESTROY,
      encryption: dynamodb.TableEncryption.AWS_MANAGED,
    });

    this.visionBucket = new s3.Bucket(this, 'VisionAssetsBucket', {
      bucketName: hyphenName(g2Env, 'vision-assets'),
      encryption: s3.BucketEncryption.KMS,
      encryptionKey: kmsKey,
      blockPublicAccess: s3.BlockPublicAccess.BLOCK_ALL,
      enforceSSL: true,
      versioned: g2Env === 'prod',
      lifecycleRules: [
        {
          id: 'TransitionToIa',
          enabled: true,
          transitions: [
            {
              storageClass: s3.StorageClass.INFREQUENT_ACCESS,
              transitionAfter: cdk.Duration.days(90),
            },
          ],
        },
      ],
      removalPolicy:
        g2Env === 'prod' ? cdk.RemovalPolicy.RETAIN : cdk.RemovalPolicy.DESTROY,
      autoDeleteObjects: g2Env !== 'prod',
    });

    new ssm.StringParameter(this, 'ParamTimestreamDatabase', {
      parameterName: ssmStoragePath(g2Env, 'timestream-database'),
      stringValue: dbName,
    });

    new ssm.StringParameter(this, 'ParamTimestreamTableEnergy', {
      parameterName: ssmStoragePath(g2Env, 'timestream-table-energy'),
      stringValue: underscoreName(g2Env, 'table_energy'),
    });

    new ssm.StringParameter(this, 'ParamShadowTableName', {
      parameterName: ssmStoragePath(g2Env, 'dynamodb-shadow-table'),
      stringValue: this.shadowTable.tableName,
    });

    new ssm.StringParameter(this, 'ParamControlLogsTableName', {
      parameterName: ssmStoragePath(g2Env, 'dynamodb-control-logs-table'),
      stringValue: this.controlLogsTable.tableName,
    });

    new ssm.StringParameter(this, 'ParamVisionBucketName', {
      parameterName: ssmStoragePath(g2Env, 's3-vision-bucket'),
      stringValue: this.visionBucket.bucketName,
    });

    new cdk.CfnOutput(this, 'TimestreamDatabase', { value: dbName });
    new cdk.CfnOutput(this, 'TimestreamTableEnergy', {
      value: underscoreName(g2Env, 'table_energy'),
    });
    new cdk.CfnOutput(this, 'ShadowTableName', {
      value: this.shadowTable.tableName,
    });
    new cdk.CfnOutput(this, 'VisionBucketName', {
      value: this.visionBucket.bucketName,
    });
  }
}
