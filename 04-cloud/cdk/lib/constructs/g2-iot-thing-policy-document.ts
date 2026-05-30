import type { G2Env } from '../config';
import { G2_DOMAIN_MQTT_SUFFIXES, G2_DOMAINS } from '../config/domains';
import { mqttTopicPrefix } from '../naming';

export interface G2IotThingPolicyDocumentProps {
  readonly g2Env: G2Env;
  readonly region: string;
  readonly account: string;
}

/**
 * IoT policy for G2 edge Things — precise topics only (no `#`).
 * Cross-env publish to the other env is denied.
 */
export function buildG2IotThingPolicyDocument(
  props: G2IotThingPolicyDocumentProps,
): Record<string, unknown> {
  const { g2Env, region, account } = props;
  const arnBase = `arn:aws:iot:${region}:${account}`;

  const publishTopics: string[] = [];
  const subscribeFilters: string[] = [];

  for (const domain of G2_DOMAINS) {
    for (const suffix of G2_DOMAIN_MQTT_SUFFIXES[domain]) {
      const topic = mqttTopicPrefix(g2Env, domain, suffix);
      if (suffix === 'command' && domain === 'control') {
        subscribeFilters.push(`${arnBase}:topicfilter/${topic}`);
      } else {
        publishTopics.push(`${arnBase}:topic/${topic}`);
      }
    }
  }

  const otherEnv: G2Env = g2Env === 'dev' ? 'prod' : 'dev';
  const denyPublishTopics = G2_DOMAINS.flatMap((domain) =>
    G2_DOMAIN_MQTT_SUFFIXES[domain].map(
      (suffix) => `${arnBase}:topic/${mqttTopicPrefix(otherEnv, domain, suffix)}`,
    ),
  );

  return {
    Version: '2012-10-17',
    Statement: [
      {
        Sid: 'G2Connect',
        Effect: 'Allow',
        Action: ['iot:Connect'],
        Resource: [`${arnBase}:client/\${iot:Connection.Thing.ThingName}`],
      },
      {
        Sid: 'G2PublishTelemetryAndEvents',
        Effect: 'Allow',
        Action: ['iot:Publish'],
        Resource: publishTopics,
      },
      {
        Sid: 'G2SubscribeControlCommand',
        Effect: 'Allow',
        Action: ['iot:Subscribe'],
        Resource: subscribeFilters,
      },
      {
        Sid: 'G2ReceiveControlCommand',
        Effect: 'Allow',
        Action: ['iot:Receive'],
        Resource: subscribeFilters.map((f) => f.replace(':topicfilter/', ':topic/')),
      },
      {
        Sid: 'G2DenyOtherEnvironment',
        Effect: 'Deny',
        Action: ['iot:Publish', 'iot:Subscribe'],
        Resource: [...denyPublishTopics, ...denyPublishTopics.map((t) => t.replace(':topic/', ':topicfilter/'))],
      },
    ],
  };
}
