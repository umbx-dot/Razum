#[no_mangle]
pub extern "C" fn rust_step(value: i32) -> i32 {
    value.wrapping_add(3)
}
