import sys

class Compressor:
    def __init__(self, in_filename, out_filename):
        self.in_filename = in_filename
        self.out_filename = out_filename
        self.in_file = open(self.in_filename, "rb")
        self.out_file = open(self.out_filename, "wb")
        self.num_words = 0

    def pack(self):
        self.out_file.write(self.num_words.to_bytes(4, "little"))
        while True:
            word = self.in_file.readline()
            word = word.replace(b'"', b'')
            word = word.replace(b'\n', b'')
            
            if len(word) == 0:
                break
            elif len(word) > 16:
                print(f"Warning: skipping long word: {word} -> {word[:16]}")
                word = word[:16]
                continue
            data = bytearray(16)
            data[:len(word)] = bytearray(word)
            self.out_file.write(data)
            self.num_words += 1
        self.out_file.seek(0)
        self.out_file.write(self.num_words.to_bytes(4, "little"))
        self.out_file.close()

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: pack_words.py <in_file> <out_file>")
        sys.exit(1)
    c = Compressor(sys.argv[1], sys.argv[2])
    c.pack()