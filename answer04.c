#include <stdlib.h>
#include <stdio.h>

#include "answer04.h"

// macros used in Is_BMP_Header_Valid function

#define BMP_TYPE 0x4d42
#define BMP_HEADER_SIZE 54
#define DIB_HEADER_SIZE 40

/* check whether a header is valid
 * assume that header has been read from fptr
 * the position of the indicator of fptr is not certain
 * could be at the beginning of the file, end of the file or 
 * anywhere in the file
 * note that the check is only for this exercise/assignment
 * in general, the format is more complicated
 */

int is_BMP_header_valid(BMP_header* header, FILE *fptr) {
	// Make sure this is a BMP file
	if (header->type != BMP_TYPE) {
		return 0;
	}
	// skip the two unused reserved fields

	// check the offset from beginning of file to image data
	// essentially the size of the BMP header
	// BMP_HEADER_SIZE for this exercise/assignment
	if (header->offset != BMP_HEADER_SIZE) {
		return 0;
	}

	// check the DIB header size == DIB_HEADER_SIZE
	// For this exercise/assignment
	if (header->DIB_header_size != DIB_HEADER_SIZE) {
		return 0;
	}

	// Make sure there is only one image plane
	if (header->planes != 1) {
		return 0;
	}
	// Make sure there is no compression
	if (header->compression != 0) {
		return 0;
	}

	// skip the test for xresolution, yresolution

	// ncolours and importantcolours should be 0
	if (header->ncolours != 0) {
		return 0;
	}
	if (header->importantcolours != 0) {
		return 0;
	}

	// Make sure we are getting 24 bits per pixel
	// or 16 bits per pixel
	if (header->bits != 24 && header->bits != 16) {
		return 0;
	}

	// fill in code to check for file size, image size
	// based on bits, width, and height

	// check for image size/row padding
	unsigned int bytes_per_row = header->width * header->bits / 8;
	int padding = 0;
	if ((bytes_per_row % 4) != 0)
	{
		padding = 4 - bytes_per_row % 4;
	}

	unsigned int image_size_bytes = (bytes_per_row + padding) * header->height;
	if (image_size_bytes != header->imagesize)
	{
		return 0; // image sizes doesn't match each other
	}

	if (header->size - BMP_HEADER_SIZE != image_size_bytes)
	{
		return 0; // image size doesn't match file size + header size
	}

	// use file pointer to find actual file size
	fseek(fptr, 0l, SEEK_SET);
	unsigned int bytes_in_file = 0;
	while(fgetc(fptr) != EOF)
	{
		bytes_in_file++;
	}
	if (bytes_in_file != header->size)
	{
		return 0; // file sizes don't match
	}

	return 1;
}

BMP_image *read_BMP_image(char *filename)
{
	FILE *fp = fopen(filename, "r");
	if (fp == NULL)
	{
		return NULL;
	}

	BMP_image *file_contents = malloc(sizeof(*file_contents));
	if (file_contents == NULL)
	{
		fclose(fp);
		return NULL;
	}

	fseek(fp, 0l, SEEK_SET);
	if (fread(&(file_contents->header), sizeof(file_contents->header), 1, fp) != 1)
	{
		free(file_contents);
		fclose(fp);
		return NULL;
	}

	file_contents->data = calloc(file_contents->header.imagesize, sizeof(*(file_contents->data)));
	if (file_contents->data == NULL)
	{
		free(file_contents);
		fclose(fp);
		return NULL;
	}

	if (fread(file_contents->data, sizeof(*(file_contents->data)), file_contents->header.imagesize, fp) != file_contents->header.imagesize)
	{
		free_BMP_image(file_contents);
		fclose(fp);
		return NULL;
	}

	// now, BMP_image struct is initialized with file values
	if (is_BMP_header_valid(&(file_contents->header), fp) == 0)
	{
		free_BMP_image(file_contents);
		fclose(fp);
		return NULL;
	}

	fclose(fp);
	return file_contents;
}

void free_BMP_image(BMP_image *image)
{
	free(image->data);
	free(image);
}

int write_BMP_image(char *filename, BMP_image *image)
{
	FILE *fp = fopen(filename, "w");
	if (fp == NULL)
	{
		return 0;
	}

	fseek(fp, 0l, SEEK_SET);
	if (fwrite(&(image->header), sizeof(image->header), 1, fp) != 1)
	{
		fclose(fp);
		return 0;
	}

	if (fwrite(image->data, sizeof(*(image->data)), image->header.imagesize, fp) != image->header.imagesize)
	{
		fclose(fp);
		return 0;
	}

	fclose(fp);
	return 1;
}

BMP_image *convert_24_to_16_BMP_image(BMP_image *image)
{
	BMP_image *converted = malloc(sizeof(*converted));
	if (converted == NULL)
	{
		return NULL;
	}

	// copy over struct an assign new values for converted
	converted->header = image->header;
	converted->header.bits = 16;
	unsigned int dest_bytes_per_row_wo_padding = converted->header.width * converted->header.bits / 8;
	int dest_padding = 0;
	if ((dest_bytes_per_row_wo_padding % 4) != 0)
	{
		dest_padding = 4 - (dest_bytes_per_row_wo_padding % 4);
	}
	converted->header.imagesize = (dest_bytes_per_row_wo_padding + dest_padding) * converted->header.height;
	converted->header.size = converted->header.imagesize + BMP_HEADER_SIZE;

	// allocate memory for new data array
	converted->data = calloc(converted->header.imagesize, sizeof(*(converted->data)));
	if (converted->data == NULL)
	{
		free(converted);
		return NULL;
	}

	// 24-bit to 16-bit conversion
	unsigned int src_bytes_per_row_wo_padding = image->header.width * image->header.bits / 8;
	int src_padding = 0;
	if ((src_bytes_per_row_wo_padding % 4) != 0)
	{
		src_padding = 4 - (src_bytes_per_row_wo_padding % 4);
	}
	int pixels_per_row = converted->header.width;
	int num_rows = converted->header.height;
	int src_bytes_per_pixel = image->header.bits / 8;
	int dest_bytes_per_pixel = converted->header.bits / 8;
	int src_bytes_per_row_w_padding = pixels_per_row * src_bytes_per_pixel + src_padding;
	int dest_bytes_per_row_w_padding = pixels_per_row * dest_bytes_per_pixel + dest_padding;

	// create first 2D array of rows
	unsigned char (*src_row)[src_bytes_per_row_w_padding] = (unsigned char (*)[src_bytes_per_row_w_padding])&(image->data[0]); 
	unsigned char (*dest_row)[dest_bytes_per_row_w_padding] = (unsigned char (*)[dest_bytes_per_row_w_padding])&(converted->data[0]);

	for (int r = 0; r < num_rows; r++)
	{
		// create second 2D array of pixels in each row
		unsigned char (*src_pixel_byte)[src_bytes_per_pixel] = (unsigned char (*)[src_bytes_per_pixel])&(src_row[r][0]);
		unsigned char (*dest_pixel_byte)[dest_bytes_per_pixel] = (unsigned char (*)[dest_bytes_per_pixel])&(dest_row[r][0]);

		for (int p = 0; p < pixels_per_row; p++)
		{
			// getting bit patterns and converting from source
			unsigned int byte2 = src_pixel_byte[p][2];
			unsigned int byte1 = src_pixel_byte[p][1];
			unsigned int byte0 = src_pixel_byte[p][0];
			unsigned int pixel = (byte2 << 16) | (byte1 << 8) | byte0;
			unsigned int red = (pixel & 0xFF0000) >> 19; // only has 5 meaningful bits now
			unsigned int green = (pixel & 0xFF00) >> 11; // only has 5 meaningful bits now
			unsigned int blue = (pixel & 0xFF) >> 3; // only has 5 meaningful bits now

			// assigning converted bit patterns to dest
			pixel = 0;
			pixel = (red << 10) | (green << 5) | blue;
			dest_pixel_byte[p][1] = (pixel & 0xFF00) >> 8;
			dest_pixel_byte[p][0] = pixel & 0xFF;
		}
	}

	return converted; // if successful, remember to free converted in main
} 

BMP_image *convert_16_to_24_BMP_image(BMP_image *image)
{
	BMP_image *converted = malloc(sizeof(*converted));
	if (converted == NULL)
	{
		return NULL;
	}

	// copy over struct an assign new values for converted
	converted->header = image->header;
	converted->header.bits = 24;
	unsigned int dest_bytes_per_row_wo_padding = converted->header.width * converted->header.bits / 8;
	int dest_padding = 0;
	if ((dest_bytes_per_row_wo_padding % 4) != 0)
	{
		dest_padding = 4 - (dest_bytes_per_row_wo_padding % 4);
	}
	converted->header.imagesize = (dest_bytes_per_row_wo_padding + dest_padding) * converted->header.height;
	converted->header.size = converted->header.imagesize + BMP_HEADER_SIZE;

	// allocate memory for new data array
	converted->data = calloc(converted->header.imagesize, sizeof(*(converted->data)));
	if (converted->data == NULL)
	{
		free(converted);
		return NULL;
	}

	// 24-bit to 16-bit conversion
	unsigned int src_bytes_per_row_wo_padding = image->header.width * image->header.bits / 8;
	int src_padding = 0;
	if ((src_bytes_per_row_wo_padding % 4) != 0)
	{
		src_padding = 4 - (src_bytes_per_row_wo_padding % 4);
	}
	int pixels_per_row = converted->header.width;
	int num_rows = converted->header.height;
	int src_bytes_per_pixel = image->header.bits / 8;
	int dest_bytes_per_pixel = converted->header.bits / 8;
	int src_bytes_per_row_w_padding = pixels_per_row * src_bytes_per_pixel + src_padding;
	int dest_bytes_per_row_w_padding = pixels_per_row * dest_bytes_per_pixel + dest_padding;

	// create first 2D array of rows
	unsigned char (*src_row)[src_bytes_per_row_w_padding] = (unsigned char (*)[src_bytes_per_row_w_padding])&(image->data[0]); 
	unsigned char (*dest_row)[dest_bytes_per_row_w_padding] = (unsigned char (*)[dest_bytes_per_row_w_padding])&(converted->data[0]);

	for (int r = 0; r < num_rows; r++)
	{
		// create second 2D array of pixels in each row
		unsigned char (*src_pixel_byte)[src_bytes_per_pixel] = (unsigned char (*)[src_bytes_per_pixel])&(src_row[r][0]);
		unsigned char (*dest_pixel_byte)[dest_bytes_per_pixel] = (unsigned char (*)[dest_bytes_per_pixel])&(dest_row[r][0]);

		for (int p = 0; p < pixels_per_row; p++)
		{
			// getting bit patterns and converting from source
			unsigned int byte1 = src_pixel_byte[p][1];
			unsigned int byte0 = src_pixel_byte[p][0];
			unsigned int pixel = (byte1 << 8) | byte0;
			unsigned int red = (pixel & 0x7C00) >> 10; 
			unsigned int green = (pixel & 0x3E0) >> 5; 
			unsigned int blue = (pixel & 0x1F); 
			// fprintf(stderr, "red: %d, green: %d, blue: %d\n", red, green, blue);
			red = (red * 255) / 31; // has 8 meaningful bits now
			green = (green * 255) / 31; // has 8 meaningful bits now
			blue = (blue * 255) / 31; // has 8 meaningful bits now
						  // fprintf(stderr, "red: %d, green: %d, blue: %d\n", red, green, blue);

						  // assigning converted bit patterns to dest
			dest_pixel_byte[p][2] = red;
			dest_pixel_byte[p][1] = green;
			dest_pixel_byte[p][0] = blue;
		}
	}

	return converted; // if successful, remember to free converted in main
}

BMP_image *convert_24_to_16_BMP_image_with_dithering(BMP_image *image)
{
	BMP_image *converted = malloc(sizeof(*converted));
	if (converted == NULL)
	{
		return NULL;
	}

	// copy over struct and assign new values for converted
	converted->header = image->header;
	converted->header.bits = 16;
	unsigned int dest_bytes_per_row_wo_padding = converted->header.width * converted->header.bits / 8;
	int dest_padding = 0;
	if ((dest_bytes_per_row_wo_padding % 4) != 0)
	{
		dest_padding = 4 - (dest_bytes_per_row_wo_padding % 4);
	}
	converted->header.imagesize = (dest_bytes_per_row_wo_padding + dest_padding) * converted->header.height;
	converted->header.size = converted->header.imagesize + BMP_HEADER_SIZE;

	unsigned int src_bytes_per_row_wo_padding = image->header.width * image->header.bits / 8;
	int src_padding = 0;
	if ((src_bytes_per_row_wo_padding % 4) != 0)
	{
		src_padding = 4 - (src_bytes_per_row_wo_padding % 4);
	}

	// allocate memory for converted data array
	converted->data = calloc(converted->header.imagesize, sizeof(*(converted->data)));
	if (converted->data == NULL)
	{
		free(converted);
		return NULL;
	}

	// allocate memory for copy of original 24-bit image arrray
	char *copy = malloc(sizeof(*copy) * image->header.imagesize);
	if (copy == NULL)
	{
		free_BMP_image(converted);
		return NULL;
	}
	for (int copy_i = 0; copy_i < image->header.imagesize; copy_i++)
	{
		copy[copy_i] = image->data[copy_i];
	}

	// 24-bit to 16-bit conversion
	int pixels_per_row = converted->header.width;
	int num_rows = converted->header.height;
	int src_bytes_per_pixel = image->header.bits / 8;
	int dest_bytes_per_pixel = converted->header.bits / 8;
	int src_bytes_per_row_w_padding = pixels_per_row * src_bytes_per_pixel + src_padding;
	int dest_bytes_per_row_w_padding = pixels_per_row * dest_bytes_per_pixel + dest_padding;
	
	// allocate memory for summed quant error array
	char *error = calloc(image->header.width * image->header.height * image->header.bits / 8, sizeof(*error));
	if (error == NULL)
	{
		free_BMP_image(converted);
		free(copy);
		return NULL;
	}

	// create first 2D array of rows
	unsigned char (*src_row)[src_bytes_per_row_w_padding] = (unsigned char (*)[src_bytes_per_row_w_padding])&(copy[0]); 
	unsigned char (*dest_row)[dest_bytes_per_row_w_padding] = (unsigned char (*)[dest_bytes_per_row_w_padding])&(converted->data[0]);
	// unsigned char (*error_row_pixel_byte)[src_bytes_per_row_wo_padding][src_bytes_per_pixel] = (unsigned char (*)[src_bytes_per_row_wo_padding][src_bytes_per_pixel])&(error[0]);
	char (*error_row_pixel_byte)[pixels_per_row][src_bytes_per_pixel] = (unsigned char (*)[pixels_per_row][src_bytes_per_pixel])&(error[0]);
	
	for (int r = num_rows - 1; r >= 0; r--)
	{
		// create second 2D array of pixels in each row
		unsigned char (*src_pixel_byte)[src_bytes_per_pixel] = (unsigned char (*)[src_bytes_per_pixel])&(src_row[r][0]);
		unsigned char (*dest_pixel_byte)[dest_bytes_per_pixel] = (unsigned char (*)[dest_bytes_per_pixel])&(dest_row[r][0]);

		for (int p = 0; p < pixels_per_row; p++) // doesn't iterate through any padding
		{
			// getting bit patterns and converting from source
			unsigned int byte2 = src_pixel_byte[p][2];
			unsigned int byte1 = src_pixel_byte[p][1];
			unsigned int byte0 = src_pixel_byte[p][0];
			unsigned int pixel = (byte2 << 16) | (byte1 << 8) | byte0;
			// fprintf(stderr, "error_row_pixel_byte[r][p][2]: %d\n", error_row_pixel_byte[r][p][2]);
			
			// add error to colors
			unsigned int red8 = ((pixel & 0xFF0000) >> 16); // 8 meaningful bits
			int red8_error = error_row_pixel_byte[r][p][2] / 16;
			if ((red8_error >> 31 == 0) && red8 > (255 - red8_error))
			{
				red8 = 255;
			}
			else if ((red8_error >> 31 == 0xFFFFFFFF) && red8 < (-1 * red8_error))
			{
				red8 = 0;
			}
			else
			{
				red8 += red8_error;
			}
			unsigned int green8 = ((pixel & 0xFF00) >> 8); // 8 meaningful bits
			int green8_error = error_row_pixel_byte[r][p][1] / 16;
			if ((green8_error >> 31 == 0) && green8 > (255 - green8_error))
			{
				green8 = 255;
			}
			else if ((green8_error >> 31 == 0xFFFFFFFF) && green8 < (-1 * green8_error))
			{
				green8 = 0;
			}
			else
			{
				green8 += green8_error;
			}
			unsigned int blue8 = (pixel & 0xFF); // 8 meaningful bits
			int blue8_error = error_row_pixel_byte[r][p][0] / 16;
			if ((blue8_error >> 31 == 0) && blue8 > (255 - blue8_error))
			{
				blue8 = 255;
			}
			else if ((blue8_error >> 31 == 0xFFFFFFFF) && blue8 < (-1 * blue8_error))
			{
				blue8 = 0;
			}
			else
			{
				blue8 += blue8_error;
			}

			// convert colors to 5-bits
			unsigned int red5 = red8 >> 3; // 5 meaningful bits
			unsigned int green5 = green8 >> 3; // 5 meaningful bits
			unsigned int blue5 = blue8 >> 3; // 5 meaningful bits

			// assigning converted bit patterns to dest
			pixel = 0;
			pixel = (red5 << 10) | (green5 << 5) | blue5;
			dest_pixel_byte[p][1] = (pixel & 0xFF00) >> 8;
			dest_pixel_byte[p][0] = pixel & 0xFF;

			// calc quant error for R G B 
			int q_error_r = red8 - (red5 * 255) / 31;
			int q_error_g = green8 - (green5 * 255) /31;
			int q_error_b = blue8 - (blue5 * 255) / 31;

			// put errors in valid pixels of error array
			if (p + 1 < pixels_per_row)
			{
				error_row_pixel_byte[r][p + 1][2] += q_error_r * 7;
				error_row_pixel_byte[r][p + 1][1] += q_error_g * 7;
				error_row_pixel_byte[r][p + 1][0] += q_error_b * 7;
			}
			if ((r - 1 >= 0) && (p - 1 >= 0))
			{
				error_row_pixel_byte[r - 1][p - 1][2] += q_error_r * 3;
				error_row_pixel_byte[r - 1][p - 1][1] += q_error_g * 3;
				error_row_pixel_byte[r - 1][p - 1][0] += q_error_b * 3;
			}
			if (r - 1 >= 0)
			{
				error_row_pixel_byte[r - 1][p][2] += q_error_r * 5;
				error_row_pixel_byte[r - 1][p][1] += q_error_g * 5;
				error_row_pixel_byte[r - 1][p][0] += q_error_b * 5;
			}
			if ((r - 1 >= 0) && (p + 1 < pixels_per_row))
			{
				error_row_pixel_byte[r - 1][p + 1][2] += q_error_r * 1;
				error_row_pixel_byte[r - 1][p + 1][1] += q_error_g * 1;
				error_row_pixel_byte[r - 1][p + 1][0] += q_error_b * 1;
			}
		}
	}

	free(copy);
	free(error);
	return converted; // if successful, remember to free converted in main
}
