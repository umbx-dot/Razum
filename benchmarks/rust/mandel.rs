fn main() {
    let size = 1200;
    let mut inside = 0i64;
    for row in 0..size {
        let ci = 2.0 * f64::from(row) / f64::from(size) - 1.0;
        for column in 0..size {
            let cr = 2.5 * f64::from(column) / f64::from(size) - 2.0;
            let (mut zr, mut zi) = (0.0, 0.0);
            let mut steps = 0;
            while steps < 200 && zr * zr + zi * zi <= 4.0 {
                let next = zr * zr - zi * zi + cr;
                zi = 2.0 * zr * zi + ci;
                zr = next;
                steps += 1;
            }
            if steps == 200 {
                inside += 1;
            }
        }
    }
    println!("{inside}");
}
