// secure_string.nl
// Secure string lifecycle helpers.

fn wipe_secret(secret: str) -> void {
    purge secret;
}

fn announce_secret_loaded() -> void {
    say "secret loaded";
}

fn use_and_wipe(secret: str, enabled: signal) -> void {
    if enabled {
        say "secret used";
    } else {
        say "secret not used";
    }
    purge secret;
}
