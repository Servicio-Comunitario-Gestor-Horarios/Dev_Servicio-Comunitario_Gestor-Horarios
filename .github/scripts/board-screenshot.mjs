import { chromium } from 'playwright';
import { writeFileSync } from 'fs';

const TOKEN = process.env.GH_TOKEN;
const OWNER = 'LuisRojas260305';
const PROJECT_NUMBER = 1;

async function graphql(query, variables = {}) {
  const res = await fetch('https://api.github.com/graphql', {
    method: 'POST',
    headers: {
      Authorization: `Bearer ${TOKEN}`,
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({ query, variables }),
  });
  const json = await res.json();
  if (json.errors) {
    console.error('GraphQL errors:', JSON.stringify(json.errors));
    process.exit(1);
  }
  return json.data;
}

async function fetchProjectData() {
  const query = `
    query($owner: String!, $number: Int!) {
      user(login: $owner) {
        projectV2(number: $number) {
          title
          items(first: 50) {
            nodes {
              id
              fieldValues(first: 10) {
                nodes {
                  ... on ProjectV2ItemFieldSingleSelectValue {
                    name
                    field { ... on ProjectV2SingleSelectField { name } }
                  }
                  ... on ProjectV2ItemFieldTextValue {
                    text
                    field { ... on ProjectV2FieldCommon { name } }
                  }
                  ... on ProjectV2ItemFieldDateValue {
                    date
                    field { ... on ProjectV2FieldCommon { name } }
                  }
                  ... on ProjectV2ItemFieldMilestoneValue {
                    milestone { title }
                    field { ... on ProjectV2FieldCommon { name } }
                  }
                  ... on ProjectV2ItemFieldAssigneeValue {
                    assignees(first: 5) {
                      nodes { login }
                    }
                    field { ... on ProjectV2FieldCommon { name } }
                  }
                  ... on ProjectV2ItemFieldRepositoryValue {
                    repository { nameWithOwner }
                    field { ... on ProjectV2FieldCommon { name } }
                  }
                  ... on ProjectV2ItemFieldLabelValue {
                    labels(first: 5) {
                      nodes { name color }
                    }
                    field { ... on ProjectV2FieldCommon { name } }
                  }
                }
              }
              content {
                ... on Issue {
                  number
                  title
                  state
                  url
                }
                ... on PullRequest {
                  number
                  title
                  state
                  url
                }
                ... on DraftIssue {
                  title
                }
              }
            }
          }
        }
      }
    }`;

  const data = await graphql(query, { owner: OWNER, number: PROJECT_NUMBER });
  const project = data.user.projectV2;
  return project;
}

function renderHtml(project) {
  const items = project.items.nodes;

  let rows = '';
  for (const item of items) {
    const content = item.content || {};
    const title = content.title || '(unknown)';
    const url = content.url || '';
    const state = content.state || '';
    const number = content.number || '';
    const type = content.__typename || 'Item';

    // Extract field values
    let status = '', assignee = '', labels = '';
    for (const fv of item.fieldValues.nodes) {
      if (fv.field?.name === 'Status') status = fv.name || '';
      if (fv.field?.name === 'Assignees' && fv.assignees) {
        assignee = fv.assignees.nodes.map(a => a.login).join(', ');
      }
      if (fv.field?.name === 'Labels' && fv.labels) {
        labels = fv.labels.nodes.map(l =>
          `<span style="background:${l.color}22;color:#${l.color};padding:2px 6px;border-radius:4px;font-size:11px">${l.name}</span>`
        ).join(' ');
      }
    }

    const stateColor = state === 'OPEN' || state === 'MERGED' ? '#2da44e' : '#8250df';
    const statusColor = status === 'Done' ? '#2da44e' :
      status === 'In review' ? '#d4920b' :
      status === 'In progress' ? '#0969da' : '#656d76';

    rows += `<tr>
      <td style="padding:8px 12px;border-bottom:1px solid #d0d7de">
        ${number ? `<span style="color:#656d76;font-size:12px">#${number}</span> ` : ''}
        ${url ? `<a href="${url}" style="color:#0969da;text-decoration:none">${title}</a>` : title}
      </td>
      <td style="padding:8px 12px;border-bottom:1px solid #d0d7de">
        ${state ? `<span style="display:inline-block;padding:2px 8px;border-radius:8px;font-size:11px;font-weight:600;background:${stateColor}22;color:${stateColor}">${state}</span>` : ''}
      </td>
      <td style="padding:8px 12px;border-bottom:1px solid #d0d7de">
        ${status ? `<span style="display:inline-block;padding:2px 8px;border-radius:8px;font-size:11px;font-weight:600;background:${statusColor}22;color:${statusColor}">${status}</span>` : ''}
      </td>
      <td style="padding:8px 12px;border-bottom:1px solid #d0d7de;font-size:13px;color:#656d76">
        ${assignee || ''}
      </td>
      <td style="padding:8px 12px;border-bottom:1px solid #d0d7de">
        ${labels || ''}
      </td>
    </tr>`;
  }

  return `<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<style>
  body { font-family: -apple-system,BlinkMacSystemFont,"Segoe UI","Noto Sans",sans-serif; margin: 0; padding: 24px; background: #f6f8fa; }
  h1 { font-size: 20px; font-weight: 600; margin: 0 0 16px 0; color: #1f2328; }
  table { width: 100%; border-collapse: collapse; background: #fff; border: 1px solid #d0d7de; border-radius: 8px; overflow: hidden; box-shadow: 0 1px 3px rgba(0,0,0,.08); }
  th { text-align: left; padding: 8px 12px; font-size: 12px; font-weight: 600; color: #656d76; background: #f6f8fa; border-bottom: 1px solid #d0d7de; }
  tr:hover td { background: #f6f8fa; }
  .meta { font-size: 12px; color: #656d76; margin-bottom: 16px; }
</style>
</head>
<body>
  <h1>${project.title}</h1>
  <div class="meta">${items.length} items &middot; ${new Date().toISOString().slice(0, 10)}</div>
  <table>
    <thead><tr>
      <th>Title</th>
      <th>State</th>
      <th>Status</th>
      <th>Assignees</th>
      <th>Labels</th>
    </tr></thead>
    <tbody>${rows}</tbody>
  </table>
</body>
</html>`;
}

async function main() {
  if (!TOKEN) {
    console.error('ERROR: GH_TOKEN environment variable is not set.');
    process.exit(1);
  }

  console.log('Fetching project data from GitHub API...');
  const project = await fetchProjectData();
  console.log(`Project: "${project.title}" (${project.items.nodes.length} items)`);

  const html = renderHtml(project);
  const htmlPath = '/tmp/board.html';
  writeFileSync(htmlPath, html);
  console.log(`HTML rendered: ${htmlPath}`);

  console.log('Launching browser for screenshot...');
  const browser = await chromium.launch({ headless: true });
  const page = await browser.newPage({ viewport: { width: 1200, height: 800 } });

  await page.goto('file://' + htmlPath, { waitUntil: 'networkidle' });
  await page.waitForTimeout(500);

  const now = new Date();
  const dateStr = now.toISOString().slice(0, 10);
  const filename = `board-${dateStr}.png`;

  await page.screenshot({ path: filename, fullPage: true });
  console.log(`Screenshot saved: ${filename}`);

  await browser.close();
}

main().catch((err) => {
  console.error(`ERROR: ${err.message}`);
  process.exit(1);
});
