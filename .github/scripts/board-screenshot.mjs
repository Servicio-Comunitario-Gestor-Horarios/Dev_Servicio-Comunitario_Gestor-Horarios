import { chromium } from 'playwright';
import { writeFileSync } from 'fs';

const BOARD_URL = 'https://github.com/orgs/Servicio-Comunitario-Gestor-Horarios/projects/1';

async function main() {
  const token = process.env.GH_TOKEN;
  if (!token) {
    console.error('ERROR: GH_TOKEN environment variable is not set.');
    process.exit(1);
  }

  const browser = await chromium.launch({ headless: true });
  const context = await browser.newContext();
  const page = await context.newPage();

  // Authenticate with GH token via cookie/header approach
  await page.setExtraHTTPHeaders({ Authorization: `Bearer ${token}` });

  console.log(`Navigating to ${BOARD_URL}...`);
  try {
    await page.goto(BOARD_URL, { waitUntil: 'networkidle', timeout: 30000 });
  } catch (err) {
    console.error(`ERROR: Failed to load board page: ${err.message}`);
    await browser.close();
    process.exit(1);
  }

  // Check for 404 or access-denied indicators
  const title = await page.title();
  if (title.includes('404') || page.url().includes('404')) {
    console.error('ERROR: Board returned 404. Check GH_TOKEN permissions (needs read:project).');
    await browser.close();
    process.exit(1);
  }

  console.log(`Board loaded. Title: ${title}`);

  // Wait for board content to render
  await page.waitForTimeout(3000);

  const now = new Date();
  const dateStr = now.toISOString().slice(0, 10);
  const filename = `board-${dateStr}.png`;

  await page.screenshot({ path: filename, fullPage: true });
  console.log(`Screenshot saved: ${filename}`);

  await browser.close();
}

main().catch((err) => {
  console.error(`ERROR: Unhandled exception: ${err.message}`);
  process.exit(1);
});
