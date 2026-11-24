fn main() {
    let mut total = 0i64;
    for i in 0i64..400_000_000 {
        total = total.wrapping_add((i * i) ^ (i >> 3));
    }
    println!("{total}");
}
