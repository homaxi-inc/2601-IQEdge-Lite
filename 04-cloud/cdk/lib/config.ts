import * as cdk from 'aws-cdk-lib';

/** G2 deployment environment — must match MQTT topic and resource naming. */
export type G2Env = 'dev' | 'prod';

const VALID_ENVS: readonly G2Env[] = ['dev', 'prod'];

/**
 * Read `-c env=dev|prod` from CDK context.
 * @default dev
 */
export function getG2Env(app: cdk.App): G2Env {
  const raw = app.node.tryGetContext('env') ?? 'dev';
  if (!VALID_ENVS.includes(raw as G2Env)) {
    throw new Error(
      `Invalid CDK context env="${raw}". Use: cdk synth -c env=dev  OR  -c env=prod`,
    );
  }
  return raw as G2Env;
}

/** AWS env for stack deployment (account/region from CLI profile). */
export function getAwsDeploymentEnv(): cdk.Environment {
  return {
    account: process.env.CDK_DEFAULT_ACCOUNT,
    region: process.env.CDK_DEFAULT_REGION ?? 'us-east-1',
  };
}
