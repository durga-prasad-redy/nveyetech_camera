# DOB Validation - Quick Reference Card

## 📋 API Quick Reference

### 1️⃣ SET_USER_DOB (Setup Required First!)
```c
// Set DOB before using protected APIs
uint8_t dob[] = "15-01-1990";
int8_t result = set_system_user_dob_l1(10, dob);
```
**Returns:** `0`=success, `-5`=invalid length, `-1/-2`=invalid format

---

### 2️⃣ FACTORY_RESET
```c
// BEFORE (OLD):
set_system_factory_reset_l1(0, NULL);

// AFTER (NEW):
uint8_t dob[] = "15-01-1990";
set_system_factory_reset_l1(10, dob);
```
**Returns:** `0`=success, `-6`=DOB not set, `-7`=wrong DOB

---

### 3️⃣ CONFIG_RESET
```c
// BEFORE (OLD):
set_system_config_reset_l1(0, NULL);

// AFTER (NEW):
uint8_t dob[] = "15-01-1990";
set_system_config_reset_l1(10, dob);
```
**Returns:** `0`=success, `-6`=DOB not set, `-7`=wrong DOB

---

### 4️⃣ SET_LOGIN
```c
// BEFORE (OLD):
uint8_t old_data[] = {'1','2','3','4'};
set_system_login_l1(4, old_data);

// AFTER (NEW):
uint8_t new_data[] = {
    4,                          // PIN length
    '1','2','3','4',            // PIN
    '1','5','-','0','1','-','1','9','9','0'  // DOB
};
set_system_login_l1(15, new_data);
```
**Returns:** `0`=success, `-6`=DOB not set, `-7`=wrong DOB

---

## 🔢 Error Codes Cheat Sheet

| Code | Meaning | Action |
|------|---------|--------|
| `0` | ✅ Success | - |
| `-1` | ❌ Format error | Check DOB format (dd-mm-yyyy) |
| `-2` | ❌ Invalid date | Check date validity (month/day) |
| `-5` | ❌ Wrong length | Must be exactly 10 bytes |
| `-6` | ❌ DOB not set | Call SET_USER_DOB first |
| `-7` | ❌ DOB mismatch | Provide correct DOB |

---

## 📏 Data Format Summary

### FACTORY_RESET
| Old | New |
|-----|-----|
| 0 bytes | 10 bytes (DOB) |

### CONFIG_RESET
| Old | New |
|-----|-----|
| 0 bytes | 10 bytes (DOB) |

### SET_LOGIN
| Old | New |
|-----|-----|
| 1-32 bytes (PIN) | 12-43 bytes (len+PIN+DOB) |

---

## ✅ DOB Format Rules

```
Format:    dd-mm-yyyy
Length:    10 characters
Example:   15-01-1990

✓ Valid:
  15-01-1990
  29-02-2020  (leap year)
  31-12-1999

✗ Invalid:
  32-01-1990  (day > 31)
  15-13-1990  (month > 12)
  29-02-2021  (not leap year)
  15/01/1990  (wrong separator)
  15-1-1990   (not zero-padded)
```

---

## 🚀 Quick Start (3 Steps)

```c
// Step 1: Set DOB (once during setup)
uint8_t dob[] = "15-01-1990";
set_system_user_dob_l1(10, dob);

// Step 2: Use DOB for protected operations
set_system_factory_reset_l1(10, dob);  // Factory reset
set_system_config_reset_l1(10, dob);   // Config reset

// Step 3: Set login with DOB
uint8_t login_data[] = {
    4, '1','2','3','4',                        // PIN
    '1','5','-','0','1','-','1','9','9','0'   // DOB
};
set_system_login_l1(15, login_data);
```

---

## 🧪 Testing Checklist

- [ ] Test with valid DOB (should succeed)
- [ ] Test with wrong DOB (should fail with -7)
- [ ] Test without DOB set (should fail with -6)
- [ ] Test with invalid format (should fail with -1/-2)
- [ ] Test with wrong length (should fail with -5)
- [ ] Test SET_LOGIN with various PIN lengths (1-32)

---

## 🔐 Security Notes

⚠️ **Current Implementation:**
- DOB stored in plaintext
- No encryption
- No rate limiting
- No audit logging

💡 **Production Recommendations:**
1. Encrypt DOB storage
2. Add rate limiting (prevent brute force)
3. Implement audit logging
4. Add account lockout after N failed attempts
5. Use HTTPS/TLS for transmission

---

## 📖 Full Documentation

- **Complete Guide:** `DOB_VALIDATION_GUIDE.md`
- **Test Examples:** `DOB_TEST_EXAMPLES.c`
- **Implementation Summary:** `DOB_IMPLEMENTATION_SUMMARY.md`

---

## 🆘 Common Issues

### Issue: Error -6 (DOB not set)
**Solution:** Call `set_system_user_dob_l1()` first

### Issue: Error -7 (DOB mismatch)
**Solution:** Verify DOB matches stored value exactly

### Issue: Error -5 (Invalid length)
**Solution:** Ensure DOB is exactly 10 bytes

### Issue: Error -1/-2 (Invalid format)
**Solution:** 
- Check format is `dd-mm-yyyy`
- Verify date is valid
- Check separators are hyphens (-)

---

**Version:** 1.0 | **Date:** 2026-01-19 | **Status:** Ready for Testing
