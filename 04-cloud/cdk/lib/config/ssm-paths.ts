import type { G2Env } from '../config';

const ROOT = '/iqedge/g2';

export function ssmFoundationPath(g2Env: G2Env, ...parts: string[]): string {
  return [ROOT, g2Env, 'foundation', ...parts].join('/');
}

export function ssmConfigPath(g2Env: G2Env, ...parts: string[]): string {
  return [ROOT, g2Env, 'config', ...parts].join('/');
}

export function ssmStoragePath(g2Env: G2Env, ...parts: string[]): string {
  return [ROOT, g2Env, 'storage', ...parts].join('/');
}

export function ssmRegistryPath(g2Env: G2Env, ...parts: string[]): string {
  return [ROOT, g2Env, 'registry', ...parts].join('/');
}
