fn main() {
    let limit = 50_000_000;
    let mut composite = vec![0u8; limit + 1];
    let mut count = 0i64;
    for i in 2..=limit {
        if composite[i] == 0 {
            count += 1;
            let mut multiple = i * i;
            while multiple <= limit {
                composite[multiple] = 1;
                multiple += i;
            }
        }
    }
    println!("{count}");
}
