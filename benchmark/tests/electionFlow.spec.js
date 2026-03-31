// tests/election-flow.spec.js
import { test, expect } from "@playwright/test";

const CLIENT_BASE = "http://localhost:5173";
const N_USERS = 5;
const TEST_TIMEOUT = 160000;        // 160000 for 16 works
const ELECTION_DEADLINE = 1;      // in minutes (meaning, the deadline is ELECTION_DEADLINE min from now)

test("election flow", async ({
  browser,
}) => {
  const contexts = [];
  const pages = [];
  const upks = [];
  const votes = []

  test.setTimeout(TEST_TIMEOUT);

  // --- STEP 0: Setup N_USERS clients and generate keys ---
  for (let i = 0; i < N_USERS; i++) {
    const ctx = await browser.newContext();
    const page = await ctx.newPage();
    // Load the SPA root first
    await page.goto(CLIENT_BASE);

    // Then navigate to Profile using the app’s link
    await page.getByRole("link", { name: /profile/i }).click();

    // Click "Create User"
    await page.getByRole("button", { name: /create user/i }).click();

    // Wait for keys to appear
    const upkEl = page.locator("p:has-text('upk:')");
    await expect(upkEl).toBeVisible({ timeout: 10_000 });

    // Extract upk
    const upkText = await upkEl.textContent();
    const upk = upkText.replace("upk:", "").trim();
    upks.push(upk);

    contexts.push(ctx);
    pages.push(page);
  }

  const creator = pages[0];
  const members = pages;

  console.log("Generated UPKs:", upks);

  // --- STEP 1: Creator creates group ---
  await creator.getByRole("link", { name: /home/i }).click();
  //await creator.goto(`${CLIENT_BASE}/`);
  await creator.getByRole("button", { name: /create group/i }).click();

  // Fill modal
  await creator.getByLabel(/insert the group name/i).fill("Test Group");
  await creator.getByLabel(/insert the username or public id/i).fill(upks.join(", ")); // add other members
  await creator.getByRole("button", { name: /submit group creation/i }).click();

  await expect(
    creator.getByText(/the group creation request was submitted/i)
  ).toBeVisible();

  // --- STEP 2: Members accept invites via Refresh ---
  for (const page of members) {
    await page.getByRole("link", { name: /home/i }).click();
    //await page.goto(`${CLIENT_BASE}/`);

    // Keep clicking Refresh until notification appears
    await expect(async () => {
      await page
        .getByTestId('refresh')
        .click({ timeout: 10_000 });
      await expect(page.getByText("Group Invite")).toBeVisible();
    }).toPass({ timeout: 20_000 });

    // Accept invite
    await page.getByRole("button", { name: /confirm and join group/i }).click();
  }

  // --- STEP 3: Creator creates election ---
  // Assume we're on Home page and groups have been loaded
  const refreshBtn = creator.getByTestId('refresh');
  await refreshBtn.click();

  const firstGroup = creator.locator("ul.groups-list li").first();

  // Grab the text content of the <p> inside the Group component
  const spkText = await firstGroup
    .locator("p:has-text('Group ID')")
    .textContent();

  // Extract the actual SPK
  const spkMatch = spkText.match(/Group ID:\s*(\S+)/);
  if (!spkMatch) throw new Error("Could not extract SPK from group");

  const realSpk = spkMatch[1];
  console.log("Extracted SPK:", realSpk);

  await creator.getByRole("link", { name: /home/i }).click();
  //await creator.goto(`${CLIENT_BASE}/`);
  await creator.getByRole("button", { name: /create election/i }).click();

  let currentDate = new Date();
  let d = new Date(currentDate.getTime() + ELECTION_DEADLINE * 60 * 1000); // 1min from now

  // Date string
  let ds =
    d.getFullYear().toString().padStart(4, "0") +
    "-" +
    (d.getMonth() + 1).toString().padStart(2, "0") +
    "-" +
    d.getDate().toString().padStart(2, "0");

  // Time string
  let ts =
    d.getHours().toString().padStart(2, "0") +
    ":" +
    d.getMinutes().toString().padStart(2, "0");

  let deadline = ds + "T" + ts;

  console.log(deadline);

  // Fill modal
    // Then use it in your election creation modal
  await creator.getByLabel(/insert the id of the group/i).fill(realSpk);
  await creator.getByLabel(/insert the question to be voted on/i).fill("Approve Budget?");
  await creator
    .getByLabel(/insert the available choices/i)
    .fill("Yes,No,Blank");
  await creator.getByLabel(/insert the election deadline/i).fill(deadline); //"2025-09-18T02:20"



  await creator
    .getByRole("button", { name: /submit election creation/i })
    .click();

  await expect(
    creator.getByText(/the election request was sent to the server/i)
  ).toBeVisible();

  // --- STEP 4: All users vote ---
  for (const page of pages) {
    await page.getByRole("link", { name: /home/i }).click();

    await expect(async () => {
      await page.getByTestId('refresh').click();
      await expect(page.getByText("Vote on Election")).toBeVisible();
    }).toPass({ timeout: 30_000 });

    await page.getByRole("button", { name: /cast vote/i }).click();

    // --- Vote modal ---
    const radios = page.getByRole("radio"); // grab all radio options
    const count = await radios.count();

    // Pick a random option
    const randomIndex = Math.floor(Math.random() * count);
    await radios.nth(randomIndex).click();
    votes.push(randomIndex);

    // Encrypt and send
    await page.getByRole("button", { name: /protect/i }).click();
    await expect(page.getByText(/your vote has been protected/i)).toBeVisible();
    await page.getByRole("button", { name: /send vote/i }).click();
  }

  // --- STEP 5: Wait until the deadline ---
  const deadlineDate = new Date(deadline);
  const now = new Date();

  // Wait time = time until deadline + 5s buffer
  let waitTime = deadlineDate.getTime() - now.getTime() + 5000;

  /*if (waitTime < 0) {
    console.warn("Deadline already passed, skipping wait.");
    waitTime = 0;
  }*/

  console.log(`Waiting ${waitTime / 1000}s until deadline has passed...`);
  //await creator.waitForTimeout(9000_000);
  await creator.waitForTimeout(1 * 60 * 1000); //10min

  // --- STEP 6: Navigate to Elections page ---
  await creator.getByRole("link", { name: /elections/i }).click();

  // Refresh to load elections
  await creator.getByTestId('refresh').click();

  // Find the election by its name
  const electionItem = creator.getByRole("link", { name: /approve budget\?/i });
  await expect(electionItem).toBeVisible({ timeout: 20_000 });

  // Click on the election to go to detail page
  await electionItem.click();

  // --- STEP 7: Check the result ---
  const resultEl = creator.getByTestId("election-result");
  await expect(resultEl).toBeVisible({ timeout: 40_000 });

  const resultText = resultEl.textContent;
  console.log("Election result:", resultText);
  console.log("Votes " + votes);

  //expect(resultText).not.toMatch(/null|undefined|$/i);

  // Cleanup
  for (const ctx of contexts) {
    await ctx.close();
  }
});
