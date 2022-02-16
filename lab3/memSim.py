import sys

def main():
    num_args = len(sys.argv)
    algo = 'FIFO'
    frame_size = 256

    # get addresses
    if num_args > 1:
        file_name = sys.argv[1]
        with open(file_name) as seq_file:
            addresses = [line.rstrip('\n') for line in seq_file]

    # get FRAME size and ALGORITHM if they exist
    for i in range(1, num_args):
        if i == 2:
            frame_size = sys.argv[i]
        elif i == 3:
            algo = sys.argv[i]


if __name__ == '__main__':
    main()