#!/usr/bin/env node
import 'source-map-support/register';
import * as cdk from 'aws-cdk-lib';
import { getAwsDeploymentEnv, getG2Env } from '../lib/config';
import { G2FoundationStack } from '../lib/stacks/g2-foundation-stack';
import { G2RegistryStack } from '../lib/stacks/g2-registry-stack';
import { G2StorageStack } from '../lib/stacks/g2-storage-stack';

const app = new cdk.App();
const g2Env = getG2Env(app);
const awsEnv = getAwsDeploymentEnv();

const foundation = new G2FoundationStack(app, `iqedge-g2-${g2Env}-foundation`, {
  g2Env,
  env: awsEnv,
  description: `IQEdge G2 Foundation (${g2Env}) — M1 KMS/SSM/IAM/IoT policy`,
});

const storage = new G2StorageStack(app, `iqedge-g2-${g2Env}-storage`, {
  g2Env,
  env: awsEnv,
  description: `IQEdge G2 Storage (${g2Env}) — M2 Timestream/DDB/S3`,
});
storage.addDependency(foundation);

const registry = new G2RegistryStack(app, `iqedge-g2-${g2Env}-registry`, {
  g2Env,
  env: awsEnv,
  description: `IQEdge G2 Registry (${g2Env}) — M3 Fleet sys_id table`,
});
registry.addDependency(storage);

app.synth();
