// tests/join-group-flow.spec.js
import { test, expect } from "@playwright/test";

const CLIENT_BASE = "http://localhost:5173";
const N_USERS = 5;                 // number of group members
const TEST_TIMEOUT = 150000;       
const WAIT_AFTER_JOIN = 30_000;    // 30s

async function setupUsers(browser) {
  const contexts = [];
  const pages = [];
  const upks = [];

  for (let i = 0; i < N_USERS + 1; i++) {
    const ctx = await browser.newContext();
    const page = await ctx.newPage();
    await page.goto(CLIENT_BASE);

    await page.getByRole("link", { name: /profile/i }).click();
    await page.getByRole("button", { name: /create user/i }).click();

    const upkEl = page.locator("p:has-text('upk:')");
    await expect(upkEl).toBeVisible({ timeout: 10_000 });

    const upkText = await upkEl.textContent();
    const upk = upkText.replace("upk:", "").trim();

    contexts.push(ctx);
    pages.push(page);
    upks.push(upk);
  }

  return { contexts, pages, upks };
}

async function createGroup(creator, upks) {
  await creator.getByRole("link", { name: /home/i }).click();
  await creator.getByRole("button", { name: /create group/i }).click();
  await creator.getByLabel(/insert the group name/i).fill("Join Test Group");

  await creator
    .getByLabel(/insert the username or public id/i)
    .fill(upks.slice(0, N_USERS).join(", "));

  await creator.getByRole("button", { name: /submit group creation/i }).click();

  await expect(
    creator.getByText(/the group creation request was submitted/i)
  ).toBeVisible();
}

async function acceptInvites(members) {
  for (const page of members) {
    await page.getByRole("link", { name: /home/i }).click();

    await expect(async () => {
      await page.getByTestId('refresh').click();
      await expect(page.getByText("Group Invite")).toBeVisible();
    }).toPass({ timeout: 30_000 });

    await page.getByRole("button", { name: /confirm and join group/i }).click();
  }
}

async function voteOnUser(members, externalUpk, voteRange) {
  const votes = [];
  for (const page of members) {
    await page.getByRole("link", { name: /home/i }).click();

    await expect(async () => {
      
      await page.getByTestId('refresh').click();
      await expect(page.getByText(/vote on user/i)).toBeVisible();
    }).toPass({ timeout: 20_000 });

    await page.getByRole("button", { name: /vote on user/i }).click();

    await page.getByLabel(/insert the username or the user's/i).fill(externalUpk);

    const randomValue =
      Math.floor(Math.random() * (voteRange.max - voteRange.min + 1)) +
      voteRange.min;

    await page.getByLabel(/insert the reputation value/i).fill(randomValue.toString());

    await page.getByRole("button", { name: /protect/i }).click();
    await expect(page.getByText(/your vote has been protected/i)).toBeVisible();
    await page.getByRole("button", { name: /send vote/i }).click();

    votes.push(randomValue);
  }
  return votes;
}

async function extractSpk(creator) {
  await creator.getByRole("link", { name: /home/i }).click();
  await creator.getByTestId('refresh').click();

  const firstGroup = creator.locator("ul.groups-list li").first();
  const spkText = await firstGroup.locator("p:has-text('Group ID')").textContent();
  const spkMatch = spkText.match(/Group ID:\s*(\S+)/);
  if (!spkMatch) throw new Error("Could not extract SPK");
  return spkMatch[1];
}

async function externalJoinGroup(externalPage, spk) {
  await externalPage.getByRole("link", { name: /groups/i }).click();
  await externalPage.getByRole("button", { name: /join group/i }).click();

  await externalPage.getByLabel(/insert the id of the group/i).fill(spk);
  await externalPage.getByRole("button", { name: /submit join request/i }).click();

  await expect(
    externalPage.getByText(/your join request was sent to the server/i)
  ).toBeVisible();
}

test("external user joins when avg score > 5", async ({ browser }) => {
  test.setTimeout(TEST_TIMEOUT);
  const { contexts, pages, upks } = await setupUsers(browser);

  const creator = pages[0];
  const members = pages.slice(0, N_USERS);
  const externalPage = pages[N_USERS];
  const externalUpk = upks[N_USERS];

  await createGroup(creator, upks);
  await acceptInvites(members);

  const votes = await voteOnUser(members, externalUpk, { min: 6, max: 10 });
  console.log("Votes (avg > 5):", votes);

  const spk = await extractSpk(creator);
  console.log("SPK:", spk);

  await externalJoinGroup(externalPage, spk);

  console.log("Waiting for join request...");
  await externalPage.waitForTimeout(WAIT_AFTER_JOIN);

  await externalPage.getByRole("link", { name: /groups/i }).click();
  await externalPage.getByTestId('refresh').click();
  await expect(externalPage.locator("ul.groups-list")).toContainText("Join Test Group");

  for (const ctx of contexts) await ctx.close();
});

test("external user denied when avg score < 5", async ({ browser }) => {
  test.setTimeout(TEST_TIMEOUT);
  const { contexts, pages, upks } = await setupUsers(browser);

  const creator = pages[0];
  const members = pages.slice(0, N_USERS);
  const externalPage = pages[N_USERS];
  const externalUpk = upks[N_USERS];

  await createGroup(creator, upks);
  await acceptInvites(members);

  const votes = await voteOnUser(members, externalUpk, { min: 1, max: 4 });
  console.log("Votes (avg < 5):", votes);

  const spk = await extractSpk(creator);
  console.log("SPK:", spk);

  await externalJoinGroup(externalPage, spk);

  console.log("Waiting for join request...");
  await externalPage.waitForTimeout(WAIT_AFTER_JOIN);

  await externalPage.getByRole("link", { name: /groups/i }).click();
  await externalPage.getByTestId('refresh').click();

  // Assert that the Groups page shows "No groups found."
  await expect(externalPage.getByText("No groups found.")).toBeVisible();

  for (const ctx of contexts) await ctx.close();
});
