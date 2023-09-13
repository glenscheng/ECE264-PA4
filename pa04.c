#include <stdio.h>
#include <stdlib.h>
#include "answer04.h"

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		return EXIT_FAILURE;
	}

	BMP_image *original = read_BMP_image(argv[1]);
	if (original == NULL)
	{
		return EXIT_FAILURE;
	}

	// fprintf(stderr, "offset: %d\n", original->header.offset);
	// fprintf(stderr, "width: %d, height: %d\n", original->header.width, original->header.height);

	if (original->header.bits == 24)
	{
		BMP_image *converted = convert_24_to_16_BMP_image_with_dithering(original);
		if (converted == NULL)
		{	
			free_BMP_image(original);
			return EXIT_FAILURE;
		}

		if (write_BMP_image(argv[2], converted) == 0)
		{
			free_BMP_image(converted);
			free_BMP_image(original);
			return EXIT_FAILURE;
		}
		
		// fprintf(stderr, "converted 24 to 16\n");
		free_BMP_image(converted);
		free_BMP_image(original);
		return EXIT_SUCCESS;
	}
	else if (original->header.bits == 16)
	{
		BMP_image *converted = convert_16_to_24_BMP_image(original);
		if (converted == NULL)
		{	
			free_BMP_image(original);
			return EXIT_FAILURE;
		}

		if (write_BMP_image(argv[2], converted) == 0)
		{
			free_BMP_image(converted);
			free_BMP_image(original);
			return EXIT_FAILURE;
		}

		// fprintf(stderr, "converted 16 to 24\n");
		free_BMP_image(converted);
		free_BMP_image(original);
		return EXIT_SUCCESS;
	}
	
	// fprintf(stderr, "bits = 16 prolly\n");
	
	free_BMP_image(original);
	return EXIT_FAILURE;
}
