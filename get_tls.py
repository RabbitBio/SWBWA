import sys

def parse_file(input_file, output_file):
    try:
        with open(input_file, 'r') as infile:
            lines = infile.readlines()
        
        offsets = []
        in_rela_slave_tls = False

        for line in lines:
            line = line.strip()
            
            # Check for the start of the .rela.slave_tls section
            if ".rela.slave_tls" in line:
                in_rela_slave_tls = True
                continue
            
            # Check for the end of the section
            if in_rela_slave_tls and ".rela.tdata" in line:
                break
            
            # Process entries in .rela.slave_tls
            if in_rela_slave_tls:
                parts = line.split()
                if len(parts) < 5:
                    continue  # Skip lines that don't have enough columns
                
                offset = parts[0]         # First column: Offset
                sym_name = parts[4]      # Fifth column: Sym. Name
                
                # Check if the Sym. Name is text1
                if sym_name == ".text1":
                    offsets.append(offset)
        
            # Write results to the output file
        with open(output_file, 'wb') as outfile:
            tls_size = len(offsets)
            print(tls_size)
            outfile.write(tls_size.to_bytes(8, byteorder='little'))  # 假设 unsigned long 是 8 字节，使用小端序
            print(offsets)

            for offset in offsets:
                outfile.write(int(offset, 16).to_bytes(8, byteorder='little'))  # 假设 offset 是十六进制字符串
        
        print(f"Processing completed. Results saved to {output_file}.")

    except FileNotFoundError:
        print(f"Error: File '{input_file}' not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python script.py <input_file>")
    else:
        input_file = sys.argv[1]
        output_file = "data.bin2"
        parse_file(input_file, output_file)

