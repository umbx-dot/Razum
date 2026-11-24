fn main() {
    let (mut best_start, mut best_length) = (0i64, 0i64);
    for start in 1i64..3_000_000 {
        let mut value = start;
        let mut length = 1;
        while value != 1 {
            if value & 1 == 0 {
                value >>= 1;
            } else {
                value = 3 * value + 1;
            }
            length += 1;
        }
        if length > best_length {
            best_length = length;
            best_start = start;
        }
    }
    println!("{best_start}\n{best_length}");
}
