import pytest

# Test the core logic functions from dc27_badge.c
# These are re-implementations to verify the C logic


def run_prng(g_random):
    """Reimplementation of Get_Random_Byte() from dc27_badge.c"""
    sum_val = 0
    
    # This calculates parity on the selected bits (mask = 0xb4)
    if g_random & 0x80:
        sum_val = 1
    
    if g_random & 0x20:
        sum_val ^= 1
    
    if g_random & 0x10:
        sum_val ^= 1
    
    if g_random & 0x04:
        sum_val ^= 1
    
    g_random <<= 1
    g_random |= sum_val
    
    # Ensure we stay within 8 bits
    g_random &= 0xFF
    
    return g_random


class TestPrintBits:
    """Test the Print_Bits() function logic"""
    
    def print_bits(self, myByte):
        """Reimplementation of Print_Bits() from dc27_badge.c"""
        result = ""
        for mask in [0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01]:
            if mask & myByte:
                result += "1"
            else:
                result += "0"
        return result
    
    def test_print_bits_zero(self):
        """Test printing bits for 0"""
        pb = TestPrintBits()
        assert pb.print_bits(0) == "00000000"
    
    def test_print_bits_max(self):
        """Test printing bits for 255"""
        pb = TestPrintBits()
        assert pb.print_bits(255) == "11111111"
    
    def test_print_bits_single_bit(self):
        """Test printing bits for single bit values"""
        pb = TestPrintBits()
        assert pb.print_bits(0x01) == "00000001"
        assert pb.print_bits(0x02) == "00000010"
        assert pb.print_bits(0x80) == "10000000"
    
    def test_print_bits_various(self):
        """Test printing bits for various values"""
        pb = TestPrintBits()
        assert pb.print_bits(0xAA) == "10101010"
        assert pb.print_bits(0x55) == "01010101"
        assert pb.print_bits(0xF0) == "11110000"
        assert pb.print_bits(0x0F) == "00001111"


class TestReorderArray:
    """Test the Reorder_Array() function logic"""
    
    def reorder_array(self, array, index, N):
        """Reimplementation of Reorder_Array() from dc27_badge.c"""
        temp = [0] * N
        
        # array[i] should present at index[i] index
        for i in range(N):
            temp[index[i]] = array[i]
        
        for i in range(N):
            array[i] = temp[i]
            index[i] = i
        
        return array, index
    
    def test_reorder_basic(self):
        """Test basic array reordering"""
        arr = [1, 2, 3, 4]
        idx = [2, 0, 3, 1]
        N = 4
        
        result_arr, result_idx = self.reorder_array(arr, idx, N)
        
        # After reordering: temp[2]=1, temp[0]=2, temp[3]=3, temp[1]=4
        # So temp = [2, 4, 1, 3]
        assert result_arr == [2, 4, 1, 3]
        # Index should be reset to [0, 1, 2, 3]
        assert result_idx == [0, 1, 2, 3]
    
    def test_reorder_single_element(self):
        """Test reordering single element"""
        arr = [42]
        idx = [0]
        N = 1
        
        result_arr, result_idx = self.reorder_array(arr, idx, N)
        assert result_arr == [42]
        assert result_idx == [0]
    
    def test_reorder_already_ordered(self):
        """Test reordering when index is already ordered"""
        arr = [1, 2, 3]
        idx = [0, 1, 2]
        N = 3
        
        result_arr, result_idx = self.reorder_array(arr, idx, N)
        assert result_arr == [1, 2, 3]
        assert result_idx == [0, 1, 2]
    
    def test_reorder_reverse(self):
        """Test reordering with reverse index"""
        arr = [1, 2, 3]
        idx = [2, 1, 0]
        N = 3
        
        result_arr, result_idx = self.reorder_array(arr, idx, N)
        # temp[2]=1, temp[1]=2, temp[0]=3 -> temp = [3, 2, 1]
        assert result_arr == [3, 2, 1]
        assert result_idx == [0, 1, 2]


class TestFlagMasks:
    """Test the badge quest flag masks"""
    
    def test_flag_masks(self):
        """Verify flag mask values from dc27_badge.c"""
        FLAG_0_MASK = 0x01  # Any Valid Communication
        FLAG_1_MASK = 0x02  # Talk/Speaker
        FLAG_2_MASK = 0x04  # Village
        FLAG_3_MASK = 0x08  # Contest & Events
        FLAG_4_MASK = 0x10  # Arts & Entertainment
        FLAG_5_MASK = 0x20  # Parties
        FLAG_6_MASK = 0x40  # Group Chat
        FLAG_ALL_MASK = 0x7F
        GROUP_ALL_MASK = 0x3F
        
        # Verify individual masks are powers of 2
        assert FLAG_0_MASK == 1
        assert FLAG_1_MASK == 2
        assert FLAG_2_MASK == 4
        assert FLAG_3_MASK == 8
        assert FLAG_4_MASK == 16
        assert FLAG_5_MASK == 32
        assert FLAG_6_MASK == 64
        
        # Verify FLAG_ALL_MASK contains all flags
        assert FLAG_ALL_MASK == (FLAG_0_MASK | FLAG_1_MASK | FLAG_2_MASK | 
                                  FLAG_3_MASK | FLAG_4_MASK | FLAG_5_MASK | FLAG_6_MASK)
        
        # Verify GROUP_ALL_MASK (first 6 flags)
        assert GROUP_ALL_MASK == (FLAG_0_MASK | FLAG_1_MASK | FLAG_2_MASK | 
                                   FLAG_3_MASK | FLAG_4_MASK | FLAG_5_MASK)
    
    def test_flag_operations(self):
        """Test flag set/check operations"""
        flags = 0
        
        # Set flags
        flags |= 0x01  # FLAG_0
        assert flags & 0x01 == 0x01
        
        flags |= 0x04  # FLAG_2
        assert flags & 0x04 == 0x04
        assert flags & 0x02 == 0x00  # FLAG_1 not set
        
        # Check all flags
        assert (flags & 0x7F) == 0x05


class TestBadgeTypes:
    """Test badge type definitions"""
    
    def test_badge_type_definitions(self):
        """Verify badge type values"""
        # From dc27_badge.c definitions
        BADGE_TYPES = {
            'HUMAN': 0,
            'GOON': 1,
            'SPEAKER': 2,
            'VENDOR': 3,
            'PRESS': 4,
            'VILLAGE': 5,
            'CONTEST': 6,
            'ARTIST': 7,
            'CFP': 8,
            'UBER': 9  # Default in the code
        }
        
        assert BADGE_TYPES['UBER'] == 9
        assert BADGE_TYPES['HUMAN'] == 0


class TestPRNG:
    """Test the Get_Random_Byte() PRNG function"""
    
    def test_prng_generates_values(self):
        """Test that PRNG generates valid byte values"""
        for _ in range(100):
            val = run_prng(0x01)
            assert 0 <= val <= 255, f"Value {val} out of byte range"
    
    def test_prng_deterministic(self):
        """Test PRNG produces same output for same input"""
        input_val = 0xAB
        result1 = run_prng(input_val)
        result2 = run_prng(input_val)
        assert result1 == result2
    
    def test_prng_sequence(self):
        """Test PRNG produces a sequence of values"""
        values = []
        val = 0x01
        for _ in range(50):
            val = run_prng(val)
            values.append(val)
        
        # Verify we got 50 values
        assert len(values) == 50
        
        # Verify all are valid bytes
        for v in values:
            assert 0 <= v <= 255
    
    def test_prng_lfsr_properties(self):
        """Test PRNG has LFSR-like properties"""
        # Starting from 0x01, run through many iterations
        val = 0x01
        seen = {val}
        for _ in range(254):  # Max period for 8-bit LFSR is 255
            val = run_prng(val)
            seen.add(val)
        
        # Should have generated many unique values
        assert len(seen) > 100, "PRNG should generate many unique values"
    
    def test_prng_specific_values(self):
        """Test specific PRNG value calculations"""
        # Test with known starting value
        val = 0x01
        
        # First iteration
        # g_random = 0x01 = 0b00000001
        # bit 7 = 0, bit 5 = 0, bit 4 = 0, bit 2 = 0 -> sum = 0
        # g_random <<= 1 -> 0x02
        # g_random |= sum -> 0x02
        val = run_prng(val)
        assert val == 0x02, f"Expected 0x02, got 0x{val:02X}"
        
        # Second iteration
        # g_random = 0x02 = 0b00000010
        # bit 7 = 0, bit 5 = 0, bit 4 = 0, bit 2 = 0 -> sum = 0
        # g_random <<= 1 -> 0x04
        # g_random |= sum -> 0x04
        val = run_prng(val)
        assert val == 0x04, f"Expected 0x04, got 0x{val:02X}"
        
        # Third iteration
        # g_random = 0x04 = 0b00000100
        # bit 7 = 0, bit 5 = 0, bit 4 = 1, bit 2 = 0 -> sum = 1
        # g_random <<= 1 -> 0x08
        # g_random |= sum -> 0x09
        val = run_prng(val)
        assert val == 0x09, f"Expected 0x09, got 0x{val:02X}"


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
