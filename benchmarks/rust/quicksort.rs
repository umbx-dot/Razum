fn quicksort(values: &mut [i64], mut low: i64, mut high: i64) {
    while low < high {
        let pivot = values[(low + (high - low) / 2) as usize];
        let (mut left, mut right) = (low, high);
        while left <= right {
            while values[left as usize] < pivot {
                left += 1;
            }
            while values[right as usize] > pivot {
                right -= 1;
            }
            if left <= right {
                values.swap(left as usize, right as usize);
                left += 1;
                right -= 1;
            }
        }
        if right - low < high - left {
            quicksort(values, low, right);
            low = left;
        } else {
            quicksort(values, left, high);
            high = right;
        }
    }
}

fn main() {
    let mut values = vec![0i64; 5_000_000];
    let mut seed = 12345u64;
    for value in &mut values {
        seed = seed.wrapping_mul(6364136223846793005).wrapping_add(1442695040888963407);
        *value = (seed >> 33) as i64;
    }
    let high = values.len() as i64 - 1;
    quicksort(&mut values, 0, high);
    let checksum: i64 = values.iter().step_by(1000).sum();
    println!("{checksum}");
}
