import * as cdk from 'aws-cdk-lib';
import { Construct } from 'constructs';
import type { G2Env } from '../config';
import { hyphenName } from '../naming';

export interface G2ScaffoldStackProps extends cdk.StackProps {
  readonly g2Env: G2Env;
}

/**
 * M0.1 placeholder stack — proves synth pipeline.
 * Replaced / extended by G2FoundationStack, G2StorageStack, etc. (M1+).
 */
export class G2ScaffoldStack extends cdk.Stack {
  constructor(scope: Construct, id: string, props: G2ScaffoldStackProps) {
    super(scope, id, props);

    const { g2Env } = props;

    cdk.Tags.of(this).add('Project', 'IQEdge-G2');
    cdk.Tags.of(this).add('Environment', g2Env);
    cdk.Tags.of(this).add('ManagedBy', 'cdk');

    new cdk.CfnOutput(this, 'G2Env', {
      value: g2Env,
      description: 'Active G2 environment context',
    });

    new cdk.CfnOutput(this, 'NamingExampleRule', {
      value: hyphenName(g2Env, 'rule-energy'),
      description: 'Example IoT Rule name per 008_Strategic_Guide',
    });
  }
}
