/*
	530landscape.c
	Defines functions to generate elevation data for a landscape meshes.
	Written by Josh Davis for Carleton College's CS311 - Computer Graphics.
    Edited by Cole Weinstein and Robbie Young.
*/


/* A landscape is simply a square array of floats, with each one giving an 
elevation. This file contains functions for generating landscapes. Before 
calling any of the randomized functions, you must seed the random number 
generator, for example by doing

    #include <time.h>
    time_t t;
	srand((unsigned)time(&t));

To turn the landscape into a mesh, use the appropriate 3D mesh initializer 
functions. */

/* Makes a flat landscape with the given elevation. */
void landFlat(int size, float *data, float elevation) {
	int i, j;
	for (i = 0; i < size; i += 1)
		for (j = 0; j < size; j += 1)
			data[i * size + j] = elevation;
}

/* Returns a random integer in [a, b]. Before using this function, call 
srand(). Warning: This is a poor-quality generator. It is not suitable for 
serious cryptographic or statistical applications. */
int landInt(int a, int b) {
	return rand() % (b - a + 1) + a;
}

/* Returns a random float in [a, b]. Before using this function, call srand(). 
Warning: This is a poor-quality generator. It is not suitable for serious 
cryptographic or statistical applications. */
float landFloat(float a, float b) {
	return a + (b - a) * (float)rand() / RAND_MAX;
}

/* Given a line y = m x + b across the landscape (with the x-axis pointing east 
and the y-axis pointing north), raises points north of the line and lowers 
points south of it (or vice-versa). */
void landFaultEastWest(
        int size, float *data, float m, float b, float raisingNorth) {
    int i, j;
    for (j = 0; j < size; j += 1)
        for (i = 0; i < size; i += 1)
            if (j > m * i + b)
                data[i * size + j] += raisingNorth;
            else if (j < m * i + b)
                data[i * size + j] -= raisingNorth;
}

/* Given a line x = m y + b across the landscape (with the x-axis pointing east 
and the y-axis pointing north), raises points east of the line and lower points 
west of it (or vice-versa). */
void landFaultNorthSouth(
        int size, float *data, float m, float b, float raisingEast) {
    int i, j;
    for (i = 0; i < size; i += 1)
        for (j = 0; j < size; j += 1)
            if (i > m * j + b)
                data[i * size + j] += raisingEast;
            else if (i < m * j + b)
                data[i * size + j] -= raisingEast;
}

/* Randomly chooses a vertical fault and slips the landscape up and down on the 
two sides of that fault. Before using this function, call srand(). */
void landFaultRandomly(int size, float *data, float magnitude) {
	int i, j, sign;
	float m, b;
	m = landFloat(-1.0, 1.0);
	sign = (2 * landInt(0, 1) - 1);
	if (landInt(0, 1) == 0) {
		// Make a line y = m x + b, such that it intersects the landscape.
		if (m > 0)
			b = landFloat(-m * (size - 1), size - 1);
		else
			b = landFloat(-m * (size - 1), size - 1 - m * (size - 1));
		float raisingNorth = magnitude * landFloat(0.5, 1.5) * sign;
		landFaultEastWest(size, data, m, b, raisingNorth);
	} else {
		// Make a line x = m y + b, such that it intersects the landscape.
		if (m > 0)
			b = landFloat(-m * (size - 1), size - 1);
		else
			b = landFloat(-m * (size - 1), size - 1 - m * (size - 1));
		float raisingEast = magnitude * landFloat(0.5, 1.5) * sign;
		landFaultEastWest(size, data, m, b, raisingEast);
	}
}

/* Blurs each non-border elevation with the eight elevations around it. */
void landBlur(int size, float *data) {
	int i, j;
	float *copy = (float *)malloc(size * size * sizeof(float));
	if (copy == NULL) {
	    fprintf(stderr, "error: landBlur: malloc failed\n");
	    return;
	}
	for (i = 1; i < size - 1; i += 1)
		for (j = 1; j < size - 1; j += 1) {
			copy[i * size + j] = 
				(data[(i) * size + (j)] + 
				data[(i + 1) * size + (j)] + 
				data[(i - 1) * size + (j)] + 
				data[(i) * size + (j + 1)] + 
				data[(i) * size + (j - 1)] + 
				data[(i + 1) * size + (j + 1)] + 
				data[(i + 1) * size + (j - 1)] + 
				data[(i - 1) * size + (j + 1)] + 
				data[(i - 1) * size + (j - 1)]) / 9.0;
		}
	for (i = 1; i < size - 1; i += 1)
		for (j = 1; j < size - 1; j += 1)
			data[i * size + j] = copy[i * size + j];
	free(copy);
}

/* Forms a Gaussian hill or valley at (x, y), with width controlled by stddev 
and height/depth controlled by raising. */
void landBump(
        int size, float *data, int x, int y, float stddev, float raising) {
    float scalar, distSq;
    scalar = -0.5 / (stddev * stddev);
    for (int i = 0; i < size; i += 1)
        for (int j = 0; j < size; j += 1) {
            distSq = (i - x) * (i - x) + (j - y) * (j - y);
            data[i * size + j] += raising * exp(scalar * distSq);
        }
}

/* Computes the min, mean, and max of the elevations. */
void landStatistics(
        int size, float *data, float *min, float *mean, float *max) {
	*min = data[0];
	*max = data[0];
	*mean = 0.0;
	int i, j;
	for (i = 0; i < size; i += 1)
		for (j = 0; j < size; j += 1) {
			*mean += data[i * size + j];
			if (data[i * size + j] < *min)
				*min = data[i * size + j];
			if (data[i * size + j] > *max)
				*max = data[i * size + j];
		}
	*mean = *mean / (size * size);
}