/*
    470mesh.c
    Creates the meshMesh struct and defines methods to implement it.
    Modified from 350mesh3D.c to use floats instead of doubles (and uint16_ts instead of ints 
	in some places). Also, no longer defines meshRender().
    Written by Josh Davis for Carleton College's CS311 - Computer Graphics.
*/


/*** Creating and destroying ***/

/* Feel free to read the struct's members, but don't write them, except through 
the accessors below such as meshSetTriangle, meshSetVertex. */
typedef struct meshMesh meshMesh;
struct meshMesh {
	int triNum, vertNum, attrDim;
	uint16_t *tri;						/* triNum * 3 ints */
	float *vert;					/* vertNum * attrDim floats */
};

/* Initializes a mesh with enough memory to hold its triangles and vertices. 
Does not actually fill in those triangles or vertices with useful data. When 
you are finished with the mesh, you must call meshFinalize to deallocate its 
backing resources. */
int meshInitialize(meshMesh *mesh, int triNum, int vertNum, int attrDim) {
	mesh->tri = (uint16_t *)malloc(triNum * 3 * sizeof(uint16_t) +
		vertNum * attrDim * sizeof(float));
	if (mesh->tri != NULL) {
		mesh->vert = (float *)&(mesh->tri[triNum * 3]);
		mesh->triNum = triNum;
		mesh->vertNum = vertNum;
		mesh->attrDim = attrDim;
	}
	return (mesh->tri == NULL);
}

/* Sets the trith triangle to have vertex indices i, j, k. */
void meshSetTriangle(meshMesh *mesh, int tri, int i, int j, int k) {
	if (0 <= tri && tri < mesh->triNum) {
		mesh->tri[3 * tri] = i;
		mesh->tri[3 * tri + 1] = j;
		mesh->tri[3 * tri + 2] = k;
	}
}

/* Returns a pointer to the trith triangle. For example:
	int *triangle13 = meshGetTrianglePointer(&mesh, 13);
	printf("%d, %d, %d\n", triangle13[0], triangle13[1], triangle13[2]); */
uint16_t *meshGetTrianglePointer(const meshMesh *mesh, int tri) {
	if (0 <= tri && tri < mesh->triNum)
		return &mesh->tri[tri * 3];
	else
		return NULL;
}

/* Sets the vertth vertex to have attributes attr. */
void meshSetVertex(meshMesh *mesh, int vert, const float attr[]) {
	int k;
	if (0 <= vert && vert < mesh->vertNum)
		for (k = 0; k < mesh->attrDim; k += 1)
			mesh->vert[mesh->attrDim * vert + k] = attr[k];
}

/* Returns a pointer to the vertth vertex. For example:
	float *vertex13 = meshGetVertexPointer(&mesh, 13);
	printf("x = %f, y = %f\n", vertex13[0], vertex13[1]); */
float *meshGetVertexPointer(const meshMesh *mesh, int vert) {
	if (0 <= vert && vert < mesh->vertNum)
		return &mesh->vert[vert * mesh->attrDim];
	else
		return NULL;
}

/* Deallocates the resources backing the mesh. This function must be called 
when you are finished using a mesh. */
void meshFinalize(meshMesh *mesh) {
	free(mesh->tri);
}



/*** Writing and reading files ***/

/* Helper function for meshInitializeFile. */
int meshFileError(
        meshMesh *mesh, FILE *file, const char *cause, const int line) {
	fprintf(stderr, "error: meshInitializeFile: %s at line %d\n", cause, line);
	fclose(file);
	meshFinalize(mesh);
	return 3;
}

/* Initializes a mesh from a mesh file. The file format is simple and custom 
(not any industry standard). It is documented at meshSaveFile. This function 
does not do as much error checking as one might like. Use it only on trusted, 
non-corrupted files, such as ones that you have recently created using 
meshSaveFile. Returns 0 on success, non-zero on failure. Don't forget to invoke 
meshFinalize when you are done using the mesh. */
int meshInitializeFile(meshMesh *mesh, const char *path) {
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		fprintf(stderr, "error: meshInitializeFile: fopen failed\n");
		return 1;
	}
	int year, month, day, triNum, vertNum, attrDim;
	// Future work: Check version.
	if (fscanf(file, "Carleton College CS 311 mesh version %d/%d/%d\n", &year, 
			&month, &day) != 3) {
		fprintf(stderr, "error: meshInitializeFile: bad header at line 1\n");
		fclose(file);
		return 1;
	}
	if (fscanf(file, "triNum %d\n", &triNum) != 1) {
		fprintf(stderr, "error: meshInitializeFile: bad triNum at line 2\n");
		fclose(file);
		return 2;
	}
	if (fscanf(file, "vertNum %d\n", &vertNum) != 1) {
		fprintf(stderr, "error: meshInitializeFile: bad vertNum at line 3\n");
		fclose(file);
		return 3;
	}
	if (fscanf(file, "attrDim %d\n", &attrDim) != 1) {
		fprintf(stderr, "error: meshInitializeFile: bad attrDim at line 4\n");
		fclose(file);
		return 4;
	}
	if (meshInitialize(mesh, triNum, vertNum, attrDim) != 0) {
		fclose(file);
		return 5;
	}
	int line = 5, j, check;
	int a, b, c;
	double v;
	uint16_t *tri;
	if (fscanf(file, "%d Triangles:\n", &check) != 1 || check != triNum)
		return meshFileError(mesh, file, "bad header", line);
	for (line = 6; line < triNum + 6; line += 1) {
		tri = meshGetTrianglePointer(mesh, line - 6);
		if (fscanf(file, "%d %d %d\n", &a, &b, &c) != 3)
			return meshFileError(mesh, file, "bad triangle", line);
		tri[0] = a; tri[1] = b; tri[2] = c;
		if (0 > tri[0] || tri[0] >= vertNum || 0 > tri[1] || tri[1] >= vertNum 
				|| 0 > tri[2] || tri[2] >= vertNum)
			return meshFileError(mesh, file, "bad index", line);
	}
	float *vert;
	if (fscanf(file, "%d Vertices:\n", &check) != 1 || check != vertNum)
		return meshFileError(mesh, file, "bad header", line);
	for (line = triNum + 7; line < triNum + 7 + vertNum; line += 1) {
		vert = meshGetVertexPointer(mesh, line - (triNum + 7));
		for (j = 0; j < attrDim; j += 1) {
			if (fscanf(file, "%lf ", &v) != 1)
				return meshFileError(mesh, file, "bad vertex", line);
			vert[j] = v;
		}
		if (fscanf(file, "\n") != 0)
			return meshFileError(mesh, file, "bad vertex", line);
	}
	// Future work: Check EOF.
	fclose(file);
	return 0;
}

/* Saves a mesh to a file in a simple custom format (not any industry 
standard). Returns 0 on success, non-zero on failure. The first line is a 
comment of the form 'Carleton College CS 311 mesh version YYYY/MM/DD'.

I now describe version 2019/01/15. The second line says 'triNum [triNum]', 
where the latter is an integer value. The third and fourth lines do the same 
for vertNum and attrDim. The fifth line says '[triNum] Triangles:'. Then there 
are triNum lines, each holding three integers between 0 and vertNum - 1 
(separated by a space). Then there is a line that says '[vertNum] Vertices:'. 
Then there are vertNum lines, each holding attrDim floating-point numbers 
(terminated by a space). */
int meshSaveFile(const meshMesh *mesh, const char *path) {
	FILE *file = fopen(path, "w");
	if (file == NULL) {
		fprintf(stderr, "error: meshSaveFile: fopen failed\n");
		return 1;
	}
	fprintf(file, "Carleton College CS 311 mesh version 2019/01/15\n");
	fprintf(file, "triNum %d\n", mesh->triNum);
	fprintf(file, "vertNum %d\n", mesh->vertNum);
	fprintf(file, "attrDim %d\n", mesh->attrDim);
	fprintf(file, "%d Triangles:\n", mesh->triNum);
	int i, j;
	uint16_t *tri;
	for (i = 0; i < mesh->triNum; i += 1) {
		tri = meshGetTrianglePointer(mesh, i);
		fprintf(file, "%d %d %d\n", tri[0], tri[1], tri[2]);
	}
	fprintf(file, "%d Vertices:\n", mesh->vertNum);
	float *vert;
	for (i = 0; i < mesh->vertNum; i += 1) {
		vert = meshGetVertexPointer(mesh, i);
		for (j = 0; j < mesh->attrDim; j += 1)
			fprintf(file, "%f ", vert[j]);
		fprintf(file, "\n");
	}
	fclose(file);
	return 0;
}


