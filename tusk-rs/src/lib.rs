mod bind;

pub fn get_max_encode_buffer_size(length: usize) -> usize {
    unsafe { bind::tusk_get_max_encode_buffer_size(length) }
}

pub struct TuskEncoder {
    delimiter: u8,
}

impl TuskEncoder {
    pub fn new(delimiter: u8) -> Self {
        Self { delimiter }
    }

    pub fn encode(&self, input: &[u8]) -> Vec<u8> {
        let len = get_max_encode_buffer_size(input.len());
        let mut buffer = vec![0; len];
        let len = unsafe {
            bind::tusk_encode(
                input.as_ptr(),
                input.len(),
                buffer.as_mut_ptr(),
                self.delimiter,
            )
        };

        println!("Length: {}", len);
        buffer.truncate(len);

        buffer
    }

    pub fn decode(&self, input: &[u8]) -> Vec<u8> {
        let mut buffer = vec![0; input.len()];
        let len = unsafe {
            bind::tusk_decode(
                input.as_ptr(),
                input.len(),
                buffer.as_mut_ptr(),
                self.delimiter,
            )
        };

        buffer.truncate(len);

        buffer
    }
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
    fn decode_test() {
        let encoder = TuskEncoder::new(0);
        let data = [1, 2, 1, 3, 4];
        let encoded = encoder.encode(&data);
        let result = encoder.decode(&encoded);
        assert_eq!(result, data);
    }

    #[test]
    fn decode_delimiter() {
        for num in 0x00..=0xff {
            let encoder = TuskEncoder::new(num);
            let data = (0x00..=0xfe).collect::<Vec<u8>>();
            let encoded = encoder.encode(&data);
            let result = encoder.decode(&encoded);
            assert_eq!(result, data, "Failed for num: {}", num);
        }
    }

    #[test]
    fn encode_delimiter() {
        let encoder = TuskEncoder::new(1);
        let result = encoder.encode(&[1, 2, 1, 3, 4]);
        let expected = [1, 2, 2, 3, 3, 4, 1];
        assert_eq!(result, expected);

        // TODO(patrik): Check 0xfe and 0xff
        for num in 0x00..=0xfd {
            let encoder = TuskEncoder::new(num);
            let data = (0x00..=0xfe).collect::<Vec<u8>>();
            let result = encoder.encode(&data);
            let mut expected = data.clone();
            expected[num as usize] = 0xff - num;
            expected.insert(0, num + 1);
            expected.push(num);
            assert_eq!(result, expected, "Failed for num: {}", num);
        }
    }

    #[test]
    fn encode00() {
        let encoder = TuskEncoder::new(0);

        let result = encoder.encode(&[1, 2, 3, 4]);
        let expected = [5, 1, 2, 3, 4, 0];
        assert_eq!(result, expected);

        let result = encoder.encode(&[1, 2, 0, 3, 4]);
        let expected = [3, 1, 2, 3, 3, 4, 0];
        assert_eq!(result, expected);

        let result = encoder.encode(&[1, 2, 0, 3, 4, 0]);
        let expected = [3, 1, 2, 3, 3, 4, 1, 0];
        assert_eq!(result, expected);
    }

    #[test]
    fn encode_size() {
        let data = (0x01..=0xfe).collect::<Vec<u8>>();
        let buffer_size = get_max_encode_buffer_size(data.len());
        let overhead = buffer_size - data.len();
        assert_eq!(overhead, 2);

        let data = (0x01..=0xff).collect::<Vec<u8>>();
        let buffer_size = get_max_encode_buffer_size(data.len());
        let overhead = buffer_size - data.len();
        assert_eq!(overhead, 3);
    }

    #[test]
    fn encode01() {
        let encoder = TuskEncoder::new(0);

        let result = encoder.encode(&[0x00]);
        let expected = [0x01, 0x01, 0x00];
        assert_eq!(result, expected);
    }

    #[test]
    fn encode02() {
        let encoder = TuskEncoder::new(0);

        let result = encoder.encode(&[0x00, 0x00]);
        let expected = [0x01, 0x01, 0x01, 0x00];
        assert_eq!(result, expected);
    }

    #[test]
    fn encode03() {
        let encoder = TuskEncoder::new(0);

        let result = encoder.encode(&[0x00, 0x11, 0x00]);
        let expected = [0x01, 0x02, 0x11, 0x01, 0x00];
        assert_eq!(result, expected);
    }

    #[test]
    fn encode04() {
        let encoder = TuskEncoder::new(0);

        let result = encoder.encode(&[0x11, 0x22, 0x00, 0x33]);
        let expected = [0x03, 0x11, 0x22, 0x02, 0x33, 0x00];
        assert_eq!(result, expected);
    }

    #[test]
    fn encode05() {
        let encoder = TuskEncoder::new(0);

        let result = encoder.encode(&[0x11, 0x22, 0x33, 0x44]);
        let expected = [0x05, 0x11, 0x22, 0x33, 0x44, 0x00];
        assert_eq!(result, expected);
    }

    #[test]
    fn encode06() {
        let encoder = TuskEncoder::new(0);

        let result = encoder.encode(&[0x11, 0x00, 0x00, 0x00]);
        let expected = [0x02, 0x11, 0x01, 0x01, 0x01, 00];
        assert_eq!(result, expected);
    }

    #[test]
    fn encode07() {
        let encoder = TuskEncoder::new(0);

        let data = (0x01..=0xfe).collect::<Vec<u8>>();
        let result = encoder.encode(&data);

        let mut expected = data.clone();
        expected.insert(0, 0xff);
        expected.push(0x00);
        assert_eq!(result, expected);
    }

    #[test]
    fn encode08() {
        let encoder = TuskEncoder::new(0);

        let data = (0x00..=0xfe).collect::<Vec<u8>>();
        let result = encoder.encode(&data);

        let mut expected = data;
        expected.remove(0);
        expected.insert(0, 0xff);
        expected.insert(0, 0x01);
        expected.push(0x00);

        assert_eq!(result, expected);
    }

    #[test]
    fn encode09() {
        let encoder = TuskEncoder::new(0);

        let data = (0x01..=0xff).collect::<Vec<u8>>();
        let result = encoder.encode(&data);

        let mut expected = data;
        expected.insert(0, 0xff);
        expected.insert(expected.len() - 1, 0x02);
        expected.push(0x00);

        assert_eq!(result, expected);
    }

    #[test]
    fn encode10() {
        let encoder = TuskEncoder::new(0);

        let data = (0x01..=0xff)
            .map(|v: u8| v.wrapping_add(1))
            .collect::<Vec<u8>>();
        let result = encoder.encode(&data);

        let mut expected = data;
        expected.pop();
        expected.insert(0, 0xff);
        expected.push(0x01);
        expected.push(0x01);
        expected.push(0x00);

        assert_eq!(result, expected);
    }

    #[test]
    fn encode11() {
        let encoder = TuskEncoder::new(0);

        let data = (0x01..=0xff)
            .map(|v: u8| v.wrapping_add(2))
            .collect::<Vec<u8>>();
        let result = encoder.encode(&data);

        let mut expected = data;
        let len = expected.len();
        expected[len - 2] = 2;
        expected.insert(0, 0xfe);
        expected.push(0x00);

        assert_eq!(result, expected);
    }
}
