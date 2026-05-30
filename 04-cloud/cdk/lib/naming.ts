import type { G2Env } from './config';
import type { G2Domain } from './config/domains';

export { G2_DOMAINS, G2_DOMAIN_MQTT_SUFFIXES } from './config/domains';

/** IoT / DDB / IAM style: iqedge-g2-{env}-rule-energy */
export function hyphenName(env: G2Env, ...parts: string[]): string {
  return ['iqedge-g2', env, ...parts].join('-');
}

/** Timestream style: iqedge_g2_dev_table_energy */
export function underscoreName(env: G2Env, ...parts: string[]): string {
  return ['iqedge_g2', env, ...parts].join('_');
}

/** IoT Rule name (AWS: letters, digits, underscore only): iqedge_g2_dev_rule_energy */
export function iotRuleName(env: G2Env, ...parts: string[]): string {
  return underscoreName(env, ...parts);
}

/** MQTT topic prefix: iqedge/g2/dev/energy/telemetry */
export function mqttTopicPrefix(env: G2Env, domain: G2Domain, suffix: string): string {
  return `iqedge/g2/${env}/${domain}/${suffix}`;
}
