#pragma once

#include <cmath>
#include <iostream>
#include <fstream>
#include <algorithm>

namespace wlt
{
	template< typename T >
	void printData_1D( 
		T* data, 
		const size_t size, 
		const bool row = false )
	{
		if ( row )
		{
			for (size_t i = 0; i < size; ++i)
			{
				std::cout << *data << std::endl;
				data++;
			}
		}
		else
		{
			for (size_t i = 0; i < size; ++i)
			{
				std::cout << *data;

				if ( i < size-1 )
				{
					std::cout << ", ";
				}
				else if ( i == size-1 )
				{
					std::cout << std::endl;
				}

				data++;
			}
		}
	}

	template <typename T>
	class Image
	{
	public:
		int x, y;
		char magic[3];
		T** data = nullptr;
	};

	template <typename T>
	void releaseImage( Image< T >& img )
	{
		for ( size_t i = 0; i < img.y; i++ )
		{
			delete [] img.data[i];
		}

		delete [] img.data;
	}

	template <typename T>
	void readImage(
		Image< T >& img,
		const char* filePath )
	{
		char auxc[5];
	    int l, c, aux, temp[3], r;
	    FILE* filePtr;

	    if ((filePtr = fopen(filePath, "r")) == NULL)
	    {
	        printf("Erro ao abrir o filePath %s\n", filePath);
	        return;
	    }

	    r = fscanf(filePtr, "%s", img.magic);
	    fgetc(filePtr);

	    r = fscanf(filePtr,"%s", auxc);
	    if (auxc[0]=='#')
	    {
	        while(fgetc(filePtr) != '\n');
	        r = fscanf (filePtr,"%d", &img.x);
	    }
	    else img.x = atoi(auxc);

		r = fscanf(filePtr,"%d", &img.y);
	    r = fscanf(filePtr, "%d", &aux);

	    if ( img.data != nullptr )
	    {
	    	releaseImage( img );
	    }

	    img.data = new float*[img.x];

	    for (int i = 0; i < img.x; i++)
	    {
	    	img.data[i] = new float[img.x];
	    }
	    
	    for (l = 0; l < img.y; l++)
	    for (c = 0; c < img.x; c++)
	    {
	        r = fscanf(filePtr, "%d", &temp[0]);
	        r = fscanf(filePtr, "%d", &temp[1]);
	        r = fscanf(filePtr, "%d", &temp[2]);
	        img.data[l][c] = ((float)temp[2] + (float)temp[1] + (float)temp[0]) / 3.0;
	    }
	}

	template <typename T>
	void writeImage(
		const Image< T >& img, 
		const char* filePath )
	{
	    std::ofstream out;

	    out.open(filePath, std::ios_base::trunc);

	    out << img.magic << std::endl;
	    out << img.x << " " << img.y << std::endl;
	    out << "255" << std::endl;

	    for (int l = 0; l < img.y; l++)
	    {
	        for (int c = 0; c < img.x; c++)
	        {
				const int value = static_cast<int>(img.data[l][c]);
	            out << value << " " << value << " " << value << std::endl;
	        }
	    }

	    out.close();
	}

	template <typename T>
	void compress( 
		Image< T >& img, 
		const float compressionQuality )
	{
		// make mirrored array of sorted pointers to detail values
		const size_t totalSize = img.x * img.y;
		float* dataPtrs[ totalSize ];
		float energy = 0.f;
		for ( size_t i = 0; i < img.x; i++ )
		{
			for ( size_t j = 0; j < img.y; j++ )
			{
				float* dataPtr = ( img.data[ i ] + j );
				dataPtrs[ i * img.y + j ] =  dataPtr;
				energy += std::abs( *dataPtr );
			}
		}
		auto sortFunc = [](float* a, float* b){ return std::abs( *a ) < std::abs( *b ); };
		std::sort( dataPtrs, dataPtrs + totalSize, sortFunc );
		
		// compression
		float threshold = energy * compressionQuality;
		float removedEnergy = 0.f;
		for ( size_t i = 0; i < totalSize; i++ )
		{
			float* valuePtr = dataPtrs[ i ];
			const float value = std::abs( *valuePtr );
			if ( removedEnergy + value < threshold )
			{
				removedEnergy += value;
				*valuePtr = 0.f;
			}
			else
			{
				break;
			}
		}
	}

	template <typename T>
	void daub_decompositionStep(
		T *v, 
		const size_t n, 
		const bool normal, 
		const bool optimalFilters = true)
	{
		if (n < 4) return;

		T v3 = T(3);
		T sqrt3 = sqrt(T(3));
		T denom;
		size_t i, j;
		const size_t half = n >> 1;

		if (normal) denom = T(4) * sqrt(T(2));
		else denom = T(4);

		T* result = new T[n];

		T a, b, c, d, t1, t2, *offset;

		if (optimalFilters)
		{
			offset = v;

			for (i = 0, j = 0; j < n - 3; j += 2, i++)
			{
				a = *(offset++);
				b = *(offset++);
				c = *(offset++);
				d = *offset;

				t1 = a - c;
				t2 = b - d;

				result[i] = (a + d + v3*(b + c) + sqrt3*(t1 + t2)) / denom;
				result[i+half] = (a - d + v3*(c - b) + sqrt3*(t2 - t1)) / denom;

				offset--;
			}

			offset = v + n - 2;
			a = *(offset++);
			b = *offset;
			c = *v;
			d = *(v + 1);

			t1 = a - c;
			t2 = b - d;

			result[i] = (a + d + v3*(b + c) + sqrt3*(t1 + t2)) / denom;
			result[i+half] = (a - d + v3*(c - b) + sqrt3*(t2 - t1)) / denom;
		}
		else
		{
			denom = T(4) * sqrt(T(2));
			T h0 = (T(1) + sqrt3) / denom;
			T h1 = (T(3) + sqrt3) / denom;
			T h2 = (T(3) - sqrt3) / denom;
			T h3 = (T(1) - sqrt3) / denom;
			T g0 = h3;
			T g1 = -h2;
			T g2 = h1;
			T g3 = -h0;

			offset = v;

			for (i = 0, j = 0; j < n - 3; j += 2, i++)
			{
				a = *(offset++);
				b = *(offset++);
				c = *(offset++);
				d = *offset;

				t1 = a - c;
				t2 = b - d;

				result[i] = a*h0 + b*h1 + c*h2 + d*h3;
				result[i+half] = a*g0 + b*g1 + c*g2 + d*g3;

				offset--;
			}

			offset = v + n - 2;
			a = *(offset++);
			b = *offset;
			c = *v;
			d = *(v + 1);

			t1 = a - c;
			t2 = b - d;

			result[i] = a*h0 + b*h1 + c*h2 + d*h3;
			result[i+half] = a*g0 + b*g1 + c*g2 + d*g3;
		}

		for (i = 0; i < n; i++)
		{
			v[i] = result[i];
		}

		delete[] result;
	}

	template <typename T>
	void daub_decomposition(
		T *vec, 
		const size_t n, 
		const bool normal, 
		const bool optimalFilters = true)
	{
		for (size_t size = n; size >= 4; size >>= 1)
		{
			daub_decompositionStep(vec, size, normal, optimalFilters);
		}
	}

	template <typename T>
	void daub_compositionStep(
		T *v, 
		const size_t n, 
		const bool normal, 
		const bool optimalFilters = true)
	{
		if (n < 4) return;

		T v3 = T(3);
		T sqrt3 = sqrt(T(3));
		T denom;
		size_t i, j;
		const size_t half = n >> 1;
		//const size_t halfPls1 = half + 1;

		if (normal) denom = T(4) * sqrt(T(2));
		else denom = T(4);

		T* result = new T[n];

		T a, b, c, d, t1, t2;
		T *offset1, *offset2;

		if (optimalFilters)
		{
			a = *(v + half - 1);
			b = *(v + n - 1);
			c = *v;
			d = *(v + half);

			t1 = c - a;
			t2 = b - d;

			result[0] = (c + d + v3*(a + b) + sqrt3*(t1 + t2)) / denom;
			result[1] = (a - b + v3*(c - d) + sqrt3*(t1 - t2)) / denom;

			for (i = 0, j = 2; i < half - 1; i++)
			{
				offset1 = v + i;
				offset2 = v + half + i;

				a = *(offset1++);
				b = *(offset2++);
				c = *(offset1++);
				d = *(offset2++);

				t1 = c - a;
				t2 = b - d;

				result[j++] = (c + d + v3*(a + b) + sqrt3*(t1 + t2)) / denom;
				result[j++] = (a - b + v3*(c - d) + sqrt3*(t1 - t2)) / denom;
			}
		}
		else
		{
			denom = T(4) * sqrt(T(2));
			T h0 = (T(1) + sqrt3) / denom;
			T h1 = (T(3) + sqrt3) / denom;
			T h2 = (T(3) - sqrt3) / denom;
			T h3 = (T(1) - sqrt3) / denom;
			T g0 = h3;
			T g1 = -h2;
			T g2 = h1;
			T g3 = -h0;

			T Ih0 = h2;
			T Ih1 = g2;
			T Ih2 = h0;
			T Ih3 = g0;
			T Ig0 = h3;
			T Ig1 = g3;
			T Ig2 = h1;
			T Ig3 = g1;

			a = *(v + half - 1);
			b = *(v + n - 1);
			c = *v;
			d = *(v + half);

			result[0] = a * Ih0 + b * Ih1 + c * Ih2 + d * Ih3;
			result[1] = a * Ig0 + b * Ig1 + c * Ig2 + d * Ig3;

			for (i = 0, j = 2; i < half - 1; i++)
			{
				offset1 = v + i;
				offset2 = v + half + i;

				a = *(offset1++);
				b = *(offset2++);
				c = *(offset1++);
				d = *(offset2++);

				result[j++] = a * Ih0 + b * Ih1 + c * Ih2 + d * Ih3;
				result[j++] = a * Ig0 + b * Ig1 + c * Ig2 + d * Ig3;
			}
		}

		for (i = 0; i < n; i++)
		{
			v[i] = result[i];
		}

		delete [] result;
	}

	template <typename T>
	void daub_composition(
		T *vec, 
		const size_t n, 
		const bool normal, 
		const bool optimalFilters = true)
	{
		for (size_t size = 4; size <= n; size <<= 1)
		{
			daub_compositionStep(vec, size, normal, optimalFilters);
		}
	}

	template <typename T>
	void daub_normalization(
		T *vec, 
		const size_t n, 
		const bool invert = false)
	{
		size_t levels = (size_t)log2((double)(n));
		T factor;
		
		for (size_t level = 1; level < levels; level++)
		{
			size_t start;
			
			if (level == 1) start = 0;
			else start = (size_t)pow(2.0, (double)(level));
			
			size_t end = (size_t)pow(2.0, (double)(level + 1));
			
			int currentLevel = (int)(levels - level);

			factor = pow(T(2), T(currentLevel) / T(2));

			if (invert)
			for (size_t i = start; i < end; i++)
				vec[i] *= factor;
			else
			for (size_t i = start; i < end; i++)
				vec[i] /= factor;
		}
	}

	template <typename T>
	void daub_standardStepNormalization(
		T **mat, 
		const size_t rows, 
		const size_t cols, 
		const bool horizontal, 
		const bool invert = false)
	{
		size_t levels = (size_t)log2((double)(rows));
		T factor;

		for (size_t level = 1; level < levels; level++)
		{
			size_t start;

			if (level == 1) start = 0;
			else start = (size_t)pow(2.0, (double)(level));

			size_t end = (size_t)pow(2.0, (double)(level + 1));

			int currentLevel = (int)(levels - level);

			factor = pow(T(2), T(currentLevel) / T(2));

			if (invert)
			{
				if (horizontal)
				{
					for (size_t i = 0; i < rows; i++)
					{
						for (size_t j = start; j < end; j++)
						{
							mat[i][j] *= factor;
						}
					}
				}
				else
				{
					for (size_t i = start; i < end; i++)
					{
						for (size_t j = 0; j < cols; j++)
						{
							mat[i][j] *= factor;
						}
					}
				}
			}
			else
			{
				if (horizontal)
				{
					for (size_t i = 0; i < rows; i++)
					{
						for (size_t j = start; j < end; j++)
						{
							mat[i][j] /= factor;
						}
					}
				}
				else
				{
					for (size_t i = start; i < end; i++)
					{
						for (size_t j = 0; j < cols; j++)
						{
							mat[i][j] /= factor;
						}
					}
				}
			}
		}
	}

	template <typename T>
	void daub_standardDecomposition(
		T **mat, 
		const size_t rows, 
		const size_t cols, 
		const bool normal, 
		const bool stepnorm = false, 
		const bool optimalFilters = true)
	{
		T *temp_row = new T[cols];
		T *temp_col = new T[rows];

		for (size_t i = 0; i < rows; i++)
		{
			for (size_t j = 0; j < cols; j++)
				temp_row[j] = mat[i][j];

			daub_decomposition(temp_row, cols, normal, optimalFilters);

			for (size_t j = 0; j < cols; j++)
				mat[i][j] = temp_row[j];
		}

		if (!normal && stepnorm && optimalFilters)
		{
			daub_standardStepNormalization(mat, rows, cols, true);
		}

		//if (normal) cout << "Normalized ";
		//cout << "Decomposition (rows):" << endl;
		//cout << endl;
		//printMatrix(mat, rows);
		//cout << endl;

		for (size_t i = 0; i < cols; i++)
		{
			for (size_t j = 0; j < rows; j++)
				temp_col[j] = mat[j][i];

			daub_decomposition(temp_col, rows, normal, optimalFilters);

			for (size_t j = 0; j < rows; j++)
				mat[j][i] = temp_col[j];
		}

		if (!normal && stepnorm && optimalFilters)
		{
			daub_standardStepNormalization(mat, rows, cols, false);
		}

		//if (normal) cout << "Normalized ";
		//cout << "Decomposition (columns):" << endl;
		//cout << endl;
		//printMatrix(mat, rows);
		//cout << endl;

		delete[] temp_row;
		delete[] temp_col;
	}

	template <typename T>
	void daub_standardComposition(
		T **mat, 
		const size_t rows, 
		const size_t cols, 
		const bool normal, 
		const bool optimalFilters = true)
	{
		T *temp_row = new T[cols];
		T *temp_col = new T[rows];

		for (size_t i = 0; i < cols; i++)
		{
			for (size_t j = 0; j < rows; j++)
				temp_col[j] = mat[j][i];

			daub_composition(temp_col, rows, normal, optimalFilters);

			for (size_t j = 0; j < rows; j++)
				mat[j][i] = temp_col[j];
		}

		//if (normal) cout << "Normalized ";
		//cout << "Composition (columns):" << endl;
		//cout << endl;
		//printMatrix(mat, rows);
		//cout << endl;

		for (size_t i = 0; i < rows; i++)
		{
			for (size_t j = 0; j < cols; j++)
				temp_row[j] = mat[i][j];

			daub_composition(temp_row, cols, normal, optimalFilters);

			for (size_t j = 0; j < cols; j++)
				mat[i][j] = temp_row[j];
		}

		//if (normal) cout << "Normalized ";
		//cout << "Composition (rows):" << endl;
		//cout << endl;
		//printMatrix(mat, rows);
		//cout << endl;

		delete[] temp_row;
		delete[] temp_col;
	}

	template <typename T>
	void daub_standardNormalization(
		T **mat, 
		const size_t n, 
		const bool invert = false)
	{
		size_t levels = (size_t)log2((double)(n));
		T factor;

		for (size_t levelR = 1; levelR < levels; levelR++)
		{
			size_t startR;

			if (levelR == 1)	startR = 0;
			else        		startR = (size_t)pow(2.0, (double)(levelR));

			size_t endR = (size_t)pow(2.0, (double)(levelR + 1));

			for (size_t row = startR; row < endR; row++)
			{
				for (size_t levelC = 1; levelC < levels; levelC++)
				{
					size_t startC;

					if (levelC == 1)	startC = 0;
					else        		startC = (size_t)pow(2.0, (double)(levelC));

					size_t endC = (size_t)pow(2.0, (double)(levelC + 1));

					for (size_t c = startC; c < endC; c++)
					{
						size_t levelSum = (2 * levels - levelR - levelC);

						if (levelSum & 1)
						{
							if (levelSum > 2)	factor = T((int)(levelSum)-1) * sqrt(T(2));
							else				factor = sqrt(T(2));
						}
						else					factor = T((int)(levelSum));

						if (factor != T(0))
						{
							if (invert)	mat[row][c] *= factor;
							else 		mat[row][c] /= factor;
						}
					}
				}
			}
		}
	}

	template <typename T>
	void daub_nonStandardDecomposition(
		Image< T >& img, 
		const bool normal = true, 
		const bool optimalFilters = true)
	{
		size_t cols = img.x;
		size_t rows = img.y;
		size_t h = rows, w = cols;
		T** mat = img.data;
		T* temp_row = new T[cols];
		T* temp_col = new T[rows];

		while (w >= 4 || h >= 4)
		{
			if (w >= 4)
			{
				for (size_t i = 0; i < h; i++)
				{
					for (size_t j = 0; j < w; j++)
						temp_row[j] = mat[i][j];

					daub_decompositionStep(temp_row, w, normal, optimalFilters);

					for (size_t j = 0; j < w; j++)
						mat[i][j] = temp_row[j];
				}
			}

			if (h >= 4)
			{
				for (size_t i = 0; i < w; i++)
				{
					for (size_t j = 0; j < h; j++)
						temp_col[j] = mat[j][i];

					daub_decompositionStep(temp_col, h, normal, optimalFilters);

					for (size_t j = 0; j < h; j++)
						mat[j][i] = temp_col[j];
				}
			}

			if (w >= 4) w /= 2;
			if (h >= 4) h /= 2;
		}

		delete [] temp_row;
		delete [] temp_col;
	}

	template <typename T>
	void daub_nonStandardComposition(
		Image< T >& img, 
		const bool normal = true, 
		const bool optimalFilters = true)
	{
		size_t r = 4, c = 4;
		size_t cols = img.x;
		size_t rows = img.y;
		T** mat = img.data;
		T *temp_row = new T[cols];
		T *temp_col = new T[rows];

		while (c <= cols || r <= rows)
		{
			if (r <= rows)
			{
				for (size_t i = 0; i < c; i++)
				{
					for (size_t j = 0; j < rows; j++)
						temp_col[j] = mat[j][i];

					daub_compositionStep(temp_col, r, normal, optimalFilters);

					for (size_t j = 0; j < rows; j++)
						mat[j][i] = temp_col[j];
				}
			}

			if (c <= cols)
			{
				for (size_t i = 0; i < r; i++)
				{
					for (size_t j = 0; j < cols; j++)
						temp_row[j] = mat[i][j];

					daub_compositionStep(temp_row, c, normal, optimalFilters);

					for (size_t j = 0; j < cols; j++)
						mat[i][j] = temp_row[j];
				}
			}

			if (c <= cols) c *= 2;
			if (r <= rows) r *= 2;
		}

		delete [] temp_row;
		delete [] temp_col;
	}

	template <typename T>
	void daub_nonStandardNormalization(
		T **mat, 
		const size_t n, 
		const bool invert = false)
	{
		T factor;
		size_t start, end;

		size_t levels = (size_t)log2((double)n);

		for (size_t i = 1; i < levels; i++)
		{
			if (i == 1) start = 0;
			else start = (size_t)pow(2.0, (double)i);
			end = (size_t)pow(2.0, (double)i + 1);

			factor = pow(T(2.0), T(levels - i));

			if (i == 1)
			{
				if (invert)
				{
					for (size_t l = 0; l < 4; l++)
					for (size_t c = 0; c < 4; c++)
						mat[l][c] *= factor;
				}
				else
				{
					for (size_t l = 0; l < 4; l++)
					for (size_t c = 0; c < 4; c++)
						mat[l][c] /= factor;
				}

				continue;
			}

			if (invert)
			{
				for (size_t l = 0; l < end / 2; l++)
				for (size_t c = start; c < end; c++)
					mat[l][c] *= factor;
			}
			else
			{
				for (size_t l = 0; l < end / 2; l++)
				for (size_t c = start; c < end; c++)
					mat[l][c] /= factor;
			}

			if (invert)
			{
				for (size_t l = start; l < end; l++)
				for (size_t c = 0; c < end; c++)
					mat[l][c] *= factor;
			}
			else
			{
				for (size_t l = start; l < end; l++)
				for (size_t c = 0; c < end; c++)
					mat[l][c] /= factor;
			}

		}
	}
}