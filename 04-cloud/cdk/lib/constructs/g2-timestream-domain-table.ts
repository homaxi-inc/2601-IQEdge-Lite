import * as timestream from 'aws-cdk-lib/aws-timestream';
import { Construct } from 'constructs';
import type { G2Domain } from '../config/domains';

export interface G2TimestreamDomainTableProps {
  readonly databaseName: string;
  readonly tableName: string;
  readonly domain: G2Domain;
  /** Memory tier retention (hours). */
  readonly memoryRetentionHours?: number;
  /** Magnetic tier retention (days). */
  readonly magneticRetentionDays?: number;
}

/**
 * One Timestream table per G2 domain — dimensions sys_id + component_id.
 */
export class G2TimestreamDomainTable extends Construct {
  public readonly table: timestream.CfnTable;

  constructor(scope: Construct, id: string, props: G2TimestreamDomainTableProps) {
    super(scope, id);

    const memoryHours = props.memoryRetentionHours ?? 24;
    const magneticDays = props.magneticRetentionDays ?? 365;

    // Timestream allows exactly one composite partition key; component_id is a record dimension.
    this.table = new timestream.CfnTable(this, 'Table', {
      databaseName: props.databaseName,
      tableName: props.tableName,
      retentionProperties: {
        MemoryStoreRetentionPeriodInHours: memoryHours.toString(),
        MagneticStoreRetentionPeriodInDays: magneticDays.toString(),
      },
      schema: {
        compositePartitionKey: [
          { type: 'DIMENSION', name: 'sys_id', enforcementInRecord: 'REQUIRED' },
        ],
      },
    });
  }
}
