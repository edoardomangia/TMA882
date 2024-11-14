// src/main.rs
// To compile: cargo build --release
// To run: mpirun -n <num_processes> ./target/release/diffusion -n <num_iterations> -d <diffusion_constant>

use mpi::traits::*;
use std::env;
use std::fs::File;
use std::io::{BufRead, BufReader};
use std::process::exit;

fn print_usage(program_name: &str) {
    println!(
        "Usage: mpirun -n <num_processes> {} -n <num_iterations> -d <diffusion_constant>",
        program_name
    );
}

fn initialize_grid(filename: &str, width: usize, height: usize, grid: &mut Vec<f32>) {
    let file = match File::open(filename) {
        Ok(f) => f,
        Err(e) => {
            eprintln!("Failed to open init file: {}", e);
            exit(1);
        }
    };

    let reader = BufReader::new(file);
    for line in reader.lines() {
        match line {
            Ok(l) => {
                let parts: Vec<&str> = l.trim().split_whitespace().collect();
                if parts.len() != 3 {
                    continue;
                }
                if let (Ok(x), Ok(y), Ok(value)) = (
                    parts[0].parse::<usize>(),
                    parts[1].parse::<usize>(),
                    parts[2].parse::<f32>(),
                ) {
                    if x < width && y < height {
                        grid[y * width + x] = value;
                    }
                }
            }
            Err(e) => {
                eprintln!("Error reading init file: {}", e);
                exit(1);
            }
        }
    }
}

fn diffuse_step(width: usize, height: usize, current: &Vec<f32>, next: &mut Vec<f32>, c: f32) {
    for j in 1..(height - 1) {
        for i in 1..(width - 1) {
            let idx = j * width + i;
            let avg = (current[(j - 1) * width + i]
                + current[(j + 1) * width + i]
                + current[j * width + (i - 1)]
                + current[j * width + (i + 1)])
                / 4.0;
            next[idx] = current[idx] + c * (avg - current[idx]);
        }
    }
}

fn main() {
    // Initialize MPI
    let universe = mpi::initialize().unwrap();
    let world = universe.world();
    let rank = world.rank();
    let size = world.size();

    // Parse command line arguments
    let args: Vec<String> = env::args().collect();
    let mut num_iterations: usize = 0;
    let mut diffusion_const: f32 = 0.0;

    let mut i = 1;
    while i < args.len() {
        if args[i] == "-n" && i + 1 < args.len() {
            num_iterations = match args[i + 1].parse::<usize>() {
                Ok(n) => n,
                Err(_) => {
                    if rank == 0 {
                        eprintln!("Invalid number for -n: {}", args[i + 1]);
                        print_usage(&args[0]);
                    }
                    exit(1);
                }
            };
            i += 2;
        } else if args[i] == "-d" && i + 1 < args.len() {
            diffusion_const = match args[i + 1].parse::<f32>() {
                Ok(d) => d,
                Err(_) => {
                    if rank == 0 {
                        eprintln!("Invalid number for -d: {}", args[i + 1]);
                        print_usage(&args[0]);
                    }
                    exit(1);
                }
            };
            i += 2;
        } else {
            if rank == 0 {
                eprintln!("Unknown or incomplete argument: {}", args[i]);
                print_usage(&args[0]);
            }
            exit(1);
        }
    }

    if num_iterations == 0 || diffusion_const <= 0.0 {
        if rank == 0 {
            eprintln!("Invalid arguments. Ensure num_iterations and diffusion_constant are positive.");
            print_usage(&args[0]);
        }
        exit(1);
    }

    // Root process reads the grid dimensions from the "init" file
    let (width, height) = if rank == 0 {
        let file = match File::open("init") {
            Ok(f) => f,
            Err(e) => {
                eprintln!("Failed to open init file: {}", e);
                exit(1);
            }
        };
        let mut reader = BufReader::new(file);
        let mut first_line = String::new();
        if let Err(e) = reader.read_line(&mut first_line) {
            eprintln!("Failed to read from init file: {}", e);
            exit(1);
        }
        let parts: Vec<&str> = first_line.trim().split_whitespace().collect();
        if parts.len() < 2 {
            eprintln!("Init file must contain width and height.");
            exit(1);
        }
        let width = match parts[0].parse::<usize>() {
            Ok(w) => w,
            Err(_) => {
                eprintln!("Invalid width in init file.");
                exit(1);
            }
        };
        let height = match parts[1].parse::<usize>() {
            Ok(h) => h,
            Err(_) => {
                eprintln!("Invalid height in init file.");
                exit(1);
            }
        };
        (width, height)
    } else {
        (0, 0)
    };

    // Broadcast width and height to all processes
    let (width, height) = world.broadcast(&width, 0).into_iter().zip(world.broadcast(&height, 0)).next().unwrap();

    let grid_size = width * height;
    let mut current_grid: Vec<f32> = vec![0.0; grid_size];
    let mut next_grid: Vec<f32> = vec![0.0; grid_size];

    // Initialize grid only on root
    if rank == 0 {
        initialize_grid("init", width, height, &mut current_grid);
    }

    // Broadcast the initial grid to all processes
    world.broadcast_into(0, &mut current_grid[..]);

    // Run diffusion steps
    for _ in 0..num_iterations {
        // Perform diffuse step
        diffuse_step(width, height, &current_grid, &mut next_grid, diffusion_const);

        // Synchronize the grids using Allreduce with SUM
        // This will sum up the next_grid from all processes
        // Since all processes compute the same next_grid, the result will be next_grid * size
        // To maintain consistency with the C code, we proceed similarly
        let mut summed_grid = vec![0.0f32; grid_size];
        world.all_reduce_into(&next_grid[..], &mut summed_grid[..], mpi::operation::SUM);

        // Update current_grid with the summed result
        current_grid.copy_from_slice(&summed_grid);

        // Swap grids
        std::mem::swap(&mut current_grid, &mut next_grid);
    }

    // Calculate local sum
    let local_sum: f64 = current_grid.iter().map(|&x| x as f64).sum();

    // Calculate global sum
    let global_sum: f64 = world.reduce(&local_sum, 0, mpi::operation::SUM);

    // Calculate average
    let average = if rank == 0 {
        global_sum / ((grid_size * size) as f64)
    } else {
        0.0
    };

    // Broadcast average to all processes
    let average = world.broadcast(&average, 0).unwrap();

    // Calculate local absolute difference sum
    let local_abs_diff_sum: f64 = current_grid
        .iter()
        .map(|&x| (x as f64 - average).abs())
        .sum();

    // Calculate global absolute difference sum
    let global_abs_diff_sum: f64 = world.reduce(&local_abs_diff_sum, 0, mpi::operation::SUM);

    // Calculate average absolute difference
    let avg_abs_diff = if rank == 0 {
        global_abs_diff_sum / ((grid_size * size) as f64)
    } else {
        0.0
    };

    // Output results on the root process
    if rank == 0 {
        println!("average: {:.6}", average);
        println!("average absolute difference: {:.6}", avg_abs_diff);
    }

    // MPI Finalization is handled automatically when `universe` is dropped
}

