mod bind;

pub fn encode(input: &[u8]) -> Vec<u8> {
    println!("Input: {:x?}", input);
    let mut buffer = vec![0; input.len() + 1];
    unsafe {
        bind::tusk_encode(input.as_ptr(), input.len(), buffer.as_mut_ptr())
    };

    buffer
}

pub fn decode(input: &[u8]) -> Vec<u8> {
    let mut buffer = vec![0; 6];
    unsafe {
        bind::tusk_decode(input.as_ptr(), input.len(), buffer.as_mut_ptr())
    };

    buffer
}

pub fn checksum(input: &[u8]) -> u16 {
    unsafe { bind::tusk_checksum(input.as_ptr(), input.len()) }
}

pub fn check_bytes(checksum: u16) -> (u8, u8) {
    let mut a: u8 = 0;
    let mut b: u8 = 0;
    unsafe {
        bind::tusk_check_bytes(checksum, &mut a, &mut b);
    }

    (a, b)
}

#[cfg(test)]
mod checksum {
    use super::*;

    #[test]
    fn checksum_test() {
        let sum = checksum(&['a' as u8, 'b' as u8, 'c' as u8, 'd' as u8]);
        assert_eq!(sum, 0xd78b);

        let sum = checksum(b"HelloWorld");
        assert_eq!(sum, 0x6100);

        let sum = checksum(b"Test");
        assert_eq!(sum, 0xdca1);

        // let data = [1, 2, 0, 4, 3];
        // let result = encode(&data);
        // println!("Result: {:x?}", result);
        //
        // let decode_result = decode(&result);
        // println!("Decode Result: {:x?}", decode_result);
    }

    #[test]
    fn checksum_check_bytes() {
        let sum = checksum(&['a' as u8, 'b' as u8, 'c' as u8, 'd' as u8]);
        let check = check_bytes(sum);
        assert_eq!(sum, 0xd78b);
        assert_eq!(check, (0x9c, 0xd7));

        let sum = checksum(b"HelloWorld");
        let check = check_bytes(sum);
        assert_eq!(sum, 0x6100);
        assert_eq!(check, (0x9e, 0x61));
    }
}

#[cfg(test)]
mod encoding {
    use super::*;

    #[test]
    fn encode00() {
        let result = encode(&[1, 2, 3, 4]);
        let expected = [5, 1, 2, 3, 4];
        assert_eq!(result, expected);

        let result = encode(&[1, 2, 0, 3, 4]);
        let expected = [3, 1, 2, 3, 3, 4];
        assert_eq!(result, expected);

        let result = encode(&[1, 2, 0, 3, 4, 0]);
        let expected = [3, 1, 2, 3, 3, 4, 1];
        assert_eq!(result, expected);
    }

    #[test]
    fn encode01() {
        // let result = encode(&[0x00]);
        // let expected = [0x01, 0x01, 0x00];
        // assert_eq!(result, expected);
    }
}
