use std::sync::{Arc, Mutex};
use std::thread;
use std::time::Duration;
use std::f64::consts::PI;
use std::fs::File;
use std::io::Write;
use std::cmp::min;
use num::complex::Complex;  // Importing Complex type from num crate

// Constants for color maps
const COLOR_MAP_RED: [u8; 7] = [255, 0, 0, 255, 255, 0, 255];
const COLOR_MAP_GREEN: [u8; 7] = [0, 255, 0, 145, 0, 125, 255];
const COLOR_MAP_BLUE: [u8; 7] = [0, 255, 0, 0, 255, 255, 0];

// Global variables
static mut IMAGE_RESOLUTION: usize = 0;
static mut POLYNOMIAL_DEGREE: usize = 0;
static mut TOTAL_THREADS: usize = 0;

// Function to compute polynomial roots
fn calculate_polynomial_roots(poly_degree: usize) -> (Vec<f64>, Vec<f64>) {
    let mut real_roots = vec![0.0; poly_degree];
    let mut imag_roots = vec![0.0; poly_degree];
    for i in 0..poly_degree {
        real_roots[i] = (2.0 * PI * i as f64 / poly_degree as f64).cos();
        imag_roots[i] = (2.0 * PI * i as f64 / poly_degree as f64).sin();
    }
    (real_roots, imag_roots)
}

// Function to map a pixel in a row to a complex number and apply Newton's method
fn perform_newton_iteration(
    poly_degree: usize,
    roots_output: &mut Vec<usize>,
    iterations_output: &mut Vec<usize>,
    row: usize,
    img_res: usize,
    real_roots: &Vec<f64>,
    imag_roots: &Vec<f64>
) {
    let _step_size = 1.0 / unsafe { TOTAL_THREADS } as f64;
    let imag_part = 2.0 - 4.0 * row as f64 / img_res as f64;
    let inv_resolution = 1.0 / img_res as f64;
    let divergence_threshold = 1_000_000_000.0;

    for col in 0..img_res {
        let mut z = Complex::new(
            -2.0 + 4.0 * (col as f64 * inv_resolution),
            imag_part,
        );
        let mut iteration = 0;

        let mut root_index = 0;
        let mut continue_iterating = true;

        while continue_iterating && iteration <= 500 {
            let origin_magnitude_sq = z.re * z.re + z.im * z.im;

            if origin_magnitude_sq < 0.000001 {
                break;
            }

            if (origin_magnitude_sq - 1.0).abs() < 0.001 {
                for r in 0..poly_degree {
                    let delta_real = z.re - real_roots[r];
                    let delta_imag = z.im - imag_roots[r];
                    let distance = delta_real * delta_real + delta_imag * delta_imag;

                    if distance < 0.000001 || z.re > divergence_threshold || z.im > divergence_threshold {
                        root_index = r;
                        continue_iterating = false;
                        break;
                    }
                }
            }

            if continue_iterating {
                match poly_degree {
                    1 => z = Complex::new(1.0, 0.0),
                    2 => {
                        z = 0.5 * (z.re / origin_magnitude_sq - (z.im / origin_magnitude_sq) * Complex::i()) + z;
                    }
                    5 => {
                        let z_inv = Complex::new(z.re / origin_magnitude_sq, -z.im / origin_magnitude_sq);
                        z = 0.2 * z_inv.powu(4) + 0.8 * z;
                    }
                    7 => {
                        let z_inv = Complex::new(z.re / origin_magnitude_sq, -z.im / origin_magnitude_sq);
                        z = (1.0 / 7.0) * z_inv.powu(6) + (6.0 / 7.0) * z;
                    }
                    _ => (),
                }
            }

            iteration += 1;
        }

        roots_output[col] = root_index;
        iterations_output[col] = iteration;
    }
}

// Function to generate output images
fn generate_output_images(
    attractors: &mut File,
    convergence: &mut File,
    roots: &[usize],
    iterations: &[usize],
    img_res: usize
) {
    let mut pixel_buffer = vec![0u8; img_res * 3];

    for (i, &root_idx) in roots.iter().enumerate() {
        pixel_buffer[i * 3] = COLOR_MAP_RED[root_idx];
        pixel_buffer[i * 3 + 1] = COLOR_MAP_GREEN[root_idx];
        pixel_buffer[i * 3 + 2] = COLOR_MAP_BLUE[root_idx];
    }
    attractors.write_all(&pixel_buffer).unwrap();

    for (i, &iteration) in iterations.iter().enumerate() {
        let color_value = min(iteration, 255) as u8;
        pixel_buffer[i * 3] = color_value;
        pixel_buffer[i * 3 + 1] = color_value;
        pixel_buffer[i * 3 + 2] = color_value;
    }
    convergence.write_all(&pixel_buffer).unwrap();
}

// Thread function for Newton's method computation
fn newton_thread_function(
    thread_id: usize,
    img_res: usize,
    poly_degree: usize,
    real_roots: Arc<Vec<f64>>,
    imag_roots: Arc<Vec<f64>>,
    root_indices: Arc<Mutex<Vec<Vec<usize>>>>,
    iteration_counts: Arc<Mutex<Vec<Vec<usize>>>>,
    rows_completed: Arc<Mutex<Vec<u8>>>
) {
    for row in (thread_id..img_res).step_by(unsafe { TOTAL_THREADS }) {
        let mut roots_row = vec![0; img_res];
        let mut iterations_row = vec![0; img_res];

        perform_newton_iteration(
            poly_degree,
            &mut roots_row,
            &mut iterations_row,
            row,
            img_res,
            &real_roots,
            &imag_roots,
        );

        let mut root_indices_lock = root_indices.lock().unwrap();
        let mut iteration_counts_lock = iteration_counts.lock().unwrap();
        let mut rows_completed_lock = rows_completed.lock().unwrap();

        root_indices_lock[row] = roots_row;
        iteration_counts_lock[row] = iterations_row;
        rows_completed_lock[row] = 1;
    }
}

// Thread function to write the image data
fn image_writer_thread(
    img_res: usize,
    attractors: Arc<Mutex<File>>,
    convergence: Arc<Mutex<File>>,
    root_indices: Arc<Mutex<Vec<Vec<usize>>>>,
    iteration_counts: Arc<Mutex<Vec<Vec<usize>>>>,
    rows_completed: Arc<Mutex<Vec<u8>>>
) {
    for row in 0..img_res {
        loop {
            let rows_completed_lock = rows_completed.lock().unwrap();
            if rows_completed_lock[row] != 0 {
                break;
            }
            drop(rows_completed_lock);
            thread::sleep(Duration::from_micros(10));
        }

        let root_indices_lock = root_indices.lock().unwrap();
        let iteration_counts_lock = iteration_counts.lock().unwrap();

        let roots_row = &root_indices_lock[row];
        let iterations_row = &iteration_counts_lock[row];

        let mut attractors_lock = attractors.lock().unwrap();
        let mut convergence_lock = convergence.lock().unwrap();

        generate_output_images(&mut attractors_lock, &mut convergence_lock, roots_row, iterations_row, img_res);
    }
}

// Main function
fn main() {

	let args: Vec<String> = std::env::args().collect();
    if args.len() != 4 {
        eprintln!("Incorrect number of arguments. Expected 3 arguments. Exiting.");
        std::process::exit(1);
    }

    let mut poly_degree: usize = 0;

    // Parse command-line arguments
    for arg in args.iter() {
        if arg.starts_with("-t") {
            // Assign directly to the global TOTAL_THREADS
            unsafe {
                TOTAL_THREADS = arg[2..].parse::<usize>().unwrap_or_else(|_| {
                    eprintln!("Error parsing number of threads.");
                    std::process::exit(1);
                });
            }
        } else if arg.starts_with("-l") {
            // Assign directly to the global IMAGE_RESOLUTION
            unsafe {
                IMAGE_RESOLUTION = arg[2..].parse::<usize>().unwrap_or_else(|_| {
                    eprintln!("Error parsing image resolution.");
                    std::process::exit(1);
                });
            }
        } else if arg.parse::<usize>().is_ok() {
            poly_degree = arg.parse::<usize>().unwrap();
        }
    }

    unsafe {
        POLYNOMIAL_DEGREE = poly_degree;
    }    

    let img_res = unsafe { IMAGE_RESOLUTION };
    let poly_degree = unsafe { POLYNOMIAL_DEGREE };

    // Compute roots of the polynomial
    let (real_roots, imag_roots) = calculate_polynomial_roots(poly_degree);
    let real_roots = Arc::new(real_roots);
    let imag_roots = Arc::new(imag_roots);

    let mut attractors_file = File::create(format!("newton_attractors_x{}.ppm", poly_degree)).unwrap();
    let mut convergence_file = File::create(format!("newton_convergence_x{}.ppm", poly_degree)).unwrap();

    write!(attractors_file, "P6\n{} {}\n255\n", img_res, img_res).unwrap();
    write!(convergence_file, "P6\n{} {}\n75\n", img_res, img_res).unwrap();

    let root_indices = Arc::new(Mutex::new(vec![vec![0; img_res]; img_res]));
    let iteration_counts = Arc::new(Mutex::new(vec![vec![0; img_res]; img_res]));
    let rows_completed = Arc::new(Mutex::new(vec![0; img_res]));

    let attractors = Arc::new(Mutex::new(attractors_file));
    let convergence = Arc::new(Mutex::new(convergence_file));

    let mut handles = vec![];

    // Launch threads for Newton's method computation
    for thread_id in 0..unsafe { TOTAL_THREADS } {
        let real_roots = Arc::clone(&real_roots);
        let imag_roots = Arc::clone(&imag_roots);
        let root_indices = Arc::clone(&root_indices);
        let iteration_counts = Arc::clone(&iteration_counts);
        let rows_completed = Arc::clone(&rows_completed);

        let handle = thread::spawn(move || {
            newton_thread_function(
                thread_id,
                img_res,
                poly_degree,
                real_roots,
                imag_roots,
                root_indices,
                iteration_counts,
                rows_completed,
            );
        });

        handles.push(handle);
    }

    // Launch thread for image writing
    let attractors = Arc::clone(&attractors);
    let convergence = Arc::clone(&convergence);
    let root_indices = Arc::clone(&root_indices);
    let iteration_counts = Arc::clone(&iteration_counts);
    let rows_completed = Arc::clone(&rows_completed);

    let writer_handle = thread::spawn(move || {
        image_writer_thread(
            img_res,
            attractors,
            convergence,
            root_indices,
            iteration_counts,
            rows_completed,
        );
    });

    for handle in handles {
        handle.join().unwrap();
    }

    writer_handle.join().unwrap();
}

