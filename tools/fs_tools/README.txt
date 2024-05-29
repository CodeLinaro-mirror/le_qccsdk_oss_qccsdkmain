Tool to generate LFS images from a source folder or list files in a LittleFS image


To use this tool, please install essential libaries:
	pip install littlefs-tools littlefs

Useage:
    lfsimg.py [-h] [-s SOURCE] [-f FILE] [--img_size IMG_SIZE] [--block_size BLOCK_SIZE]

Options:

  -h, --help            show this help message and exit

  -s SOURCE, --source SOURCE

                        Source path. If provided, create a LittleFS image from the source folder. If not provided, the image file(-f) must be provided for parsing.

  -f FILE, --file FILE  Image file name. If creating an image and not provided, defaults to 'lfsimg.bin'.

  --img_size IMG_SIZE   Size of the LFS image (defaults to 65536(64K))

  --block_size BLOCK_SIZE

                        Block size of the LFS image (defaults to 4096)

Example:

	To generate LFS image:

		python lfsimg.py -f image-name  -s source-dir

		e.g., python lfsimg.py -f lfsimg.bin -s lfs-dir
	
	To parse LFS image:
		python lfsimg.py -f image-name