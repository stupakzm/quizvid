# Issues

## [ ] Instagram posting fails with error 400 (subcode 33)

**Status:** TODO

**Observed:** 2026-03-19

**Error:**
```
Failed to post to Instagram: Instagram container creation failed 400:
{"error":{"message":"Unsupported post request. Object with ID '***' does not exist,
cannot be loaded due to missing permissions, or does not support this operation.",
"type":"GraphMethodException","code":100,"error_subcode":33}}
```

**Cause:** Instagram Graph API returns code 100 / subcode 33 — "Object does not exist or missing permissions". This means either:
1. `INSTAGRAM_ACCESS_TOKEN` has expired (long-lived tokens expire after 60 days — most likely cause)
2. `INSTAGRAM_USER_ID` is wrong (must be the Instagram Business/Creator account numeric ID, not a Facebook Page ID)
3. The Instagram account was disconnected from its Facebook Page

**Fix:**
1. Go to Meta for Developers → your app → Graph API Explorer
2. Generate a new long-lived token with `instagram_basic` and `instagram_content_publish` permissions
3. Verify the user ID: call `GET /me/accounts` → find connected page → `GET /{page-id}?fields=instagram_business_account`
4. Update `INSTAGRAM_ACCESS_TOKEN` (and `INSTAGRAM_USER_ID` if needed) in GitHub → Settings → Secrets → Actions
