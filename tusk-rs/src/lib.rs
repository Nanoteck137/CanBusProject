// extern "C" {
//     fn tusk_get_encode_size(data_length: usize) -> usize;
//     fn tusk_encode(
//         input_buffer: *const u8,
//         input_length: usize,
//         output: *mut u8,
//     );
//     fn tusk_decode(
//         input_buffer: *const u8,
//         input_length: usize,
//         output: *mut u8,
//     );
//
//     fn tusk_checksum(buffer: *const u8, length: usize) -> u16;
//     fn tusk_check_bytes(checksum: u16, a: *mut u8, b: *mut u8);
// }

#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

pub fn encode(input: &[u8]) -> Vec<u8> {
    println!("Input: {:x?}", input);
    let mut buffer = vec![0; input.len() + 1];
    unsafe { tusk_encode(input.as_ptr(), input.len(), buffer.as_mut_ptr()) };

    buffer
}

pub fn decode(input: &[u8]) -> Vec<u8> {
    let mut buffer = vec![0; 6];
    unsafe { tusk_decode(input.as_ptr(), input.len(), buffer.as_mut_ptr()) };

    buffer
}

pub fn checksum(input: &[u8]) -> u16 {
    unsafe { tusk_checksum(input.as_ptr(), input.len()) }
}

pub fn check_bytes(checksum: u16) -> (u8, u8) {
    let mut a: u8 = 0;
    let mut b: u8 = 0;
    unsafe {
        tusk_check_bytes(checksum, &mut a, &mut b);
    }

    (a, b)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn lel() {
        let data = [1, 2, 0, 4, 3];
        let result = encode(&data);
        println!("Result: {:x?}", result);

        let decode_result = decode(&result);
        println!("Decode Result: {:x?}", decode_result);

        let sum = checksum(&['a' as u8, 'b' as u8, 'c' as u8, 'd' as u8]);
        println!("Checksum: {:#x}", sum);

        let check = check_bytes(sum);
        println!("Check Bytes: {:x?}", check);

        let sum = checksum(&[
            'a' as u8, 'b' as u8, 'c' as u8, 'd' as u8, check.0, check.1,
        ]);
        println!("Checksum: {:#x}", sum);
    }
}
