use chrono::{Duration, TimeZone, Utc};
use ed25519_dalek::PublicKey;
use v3_license_parser::{LicenseType, SignedLicense};

include!(concat!(env!("OUT_DIR"), "/publickey.rs"));

#[cxx::bridge]
mod ffi {
    #[namespace = "shared"]
    struct LicenseData {
        license_type: String,
        expiration: String,
        warn: String,
        user_id: u32,
        perpetual: bool,
        trial: bool,
        valid: bool,
        computer_id: String,
    }

    extern "Rust" {
        fn validate_license(license_str: &str) -> LicenseData;
    }
}

fn type_string(l_type: LicenseType) -> String {
    use LicenseType::*;
    match l_type {
        Basic => "basic".to_owned(),
        Pro => "pro".to_owned(),
        ChinaBasic => "basic_china".to_owned(),
        ChinaPro => "pro_china".to_owned(),
        Business => "business".to_owned(),
    }
}

fn validate_license(license_str: &str) -> ffi::LicenseData {
    if let Ok(result) = validate_license_res(license_str) {
        result
    } else {
        ffi::LicenseData {
            license_type: String::from(""),
            expiration: String::from(""),
            warn: String::from(""),
            user_id: 0,
            perpetual: false,
            trial: false,
            computer_id: String::from(""),
            valid: false,
        }
    }
}

fn validate_license_res(license_str: &str) -> Result<ffi::LicenseData, Box<dyn std::error::Error>> {
    let verifier: PublicKey = PublicKey::from_bytes(&PUBLIC_KEY)?;
    let signed_license: SignedLicense = license_str.parse()?;
    let license = &signed_license.license;
    let expiration_date = Utc
        .ymd(
            license.expiration_year.into(),
            license.expiration_month.into(),
            license.expiration_day.into(),
        )
        .and_hms(0, 0, 0);
    let expiration = expiration_date.timestamp().to_string();
    let warn_date = (expiration_date - Duration::days(30))
        .timestamp()
        .to_string();
    if signed_license.verify(verifier) {
        Ok(ffi::LicenseData {
            license_type: type_string(license.license_type),
            expiration: expiration,
            warn: warn_date,
            user_id: license.user_id,
            perpetual: license.perpetual,
            trial: license.trial,
            computer_id: license.computer_id.clone().unwrap_or_default(),
            valid: true,
        })
    } else {
        Ok(ffi::LicenseData {
            license_type: type_string(license.license_type),
            expiration: expiration,
            warn: warn_date,
            user_id: license.user_id,
            perpetual: license.perpetual,
            trial: license.trial,
            computer_id: license.computer_id.clone().unwrap_or_default(),
            valid: false,
        })
    }
}
