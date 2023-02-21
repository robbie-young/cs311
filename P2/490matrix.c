/*
    490matrix.c
    Interface for performing operations on matrices.
    Modified from 280matrix.c to use floats instead of doubles.
    Designed by Josh Davis for Carleton College's CS311 - Computer Graphics.
    Implementations written by Cole Weinstein and Robbie Young.
*/


/*** 2 x 2 Matrices ***/

/* Pretty-prints the given matrix, with one line of text per row of matrix. */
void mat22Print(const float m[2][2]) {
    int i, j;
    for (i = 0; i < 2; i += 1) {
        for (j = 0; j < 2; j += 1)
            printf("%f    ", m[i][j]);
        printf("\n");
    }
}

/* Returns the determinant of the matrix m. If the determinant is 0.0, then the 
matrix is not invertible, and mInv is untouched. If the determinant is not 0.0, 
then the matrix is invertible, and its inverse is placed into mInv. The output 
CANNOT safely alias the input. */
float mat22Invert(const float m[2][2], float mInv[2][2]) {
    float det = m[0][0]*m[1][1] - m[0][1]*m[1][0];
    if (det != 0){
        mInv[0][0] = m[1][1] / det;
        mInv[0][1] = -m[0][1] / det;
        mInv[1][0] = -m[1][0] / det;
        mInv[1][1] = m[0][0] / det;
    }
    return det;
}

/* Multiplies a 2x2 matrix m by a 2-column v, storing the result in mTimesV. 
The output CANNOT safely alias the input. */
void mat221Multiply(const float m[2][2], const float v[2], 
        float mTimesV[2]) {
    mTimesV[0] = m[0][0]*v[0] + m[0][1]*v[1];
    mTimesV[1] = m[1][0]*v[0] + m[1][1]*v[1];
}

/* Fills the matrix m from its two columns. The output CANNOT safely alias the 
input. */
void mat22Columns(const float col0[2], const float col1[2], float m[2][2]) {
    m[0][0] = col0[0];
    m[0][1] = col1[0];
    m[1][0] = col0[1];
    m[1][1] = col1[1];
}

/* The theta parameter is an angle in radians. Sets the matrix m to the 
rotation matrix corresponding to counterclockwise rotation of the plane through 
the angle theta. */
void mat22Rotation(float theta, float m[2][2]) {
    m[0][0] = cos(theta);
    m[0][1] = (-1)*sin(theta);
    m[1][0] = sin(theta);
    m[1][1] = cos(theta);
}


/*** 3 x 3 Matrices ***/

/* Multiplies the 3x3 matrix m by the 3x3 matrix n. The output CANNOT safely 
alias the input. */
void mat333Multiply(
        const float m[3][3], const float n[3][3], float mTimesN[3][3]) {
    for (int i = 0 ; i < 3 ; i++) {
        for (int j = 0 ; j < 3 ; j++) {
            mTimesN[i][j] = 0;
            for (int k = 0 ; k < 3 ; k++) {
                mTimesN[i][j] += m[i][k] * n[k][j];
            }
        }
    }
}
/* Multiplies the 3x3 matrix m by the 3x1 matrix v. The output CANNOT safely 
alias the input. */
void mat331Multiply(
        const float m[3][3], const float v[3], float mTimesV[3]) {
    mTimesV[0] = m[0][0]*v[0] + m[0][1]*v[1] + m[0][2]*v[2];
    mTimesV[1] = m[1][0]*v[0] + m[1][1]*v[1] + m[1][2]*v[2];
    mTimesV[2] = m[2][0]*v[0] + m[2][1]*v[1] + m[2][2]*v[2];
}

/* Computes the transpose M^T of the given 3x3 matrix M. The output CANNOT safely 
alias the input. */
void mat33Transpose(const float m[3][3], float mT[3][3]) {
    for (int i = 0; i < 3; i += 1)
        for (int j = 0; j < 3; j += 1)
            mT[i][j] = m[j][i];
}

/* Builds a 3x3 matrix representing 2D rotation and translation in homogeneous 
coordinates. More precisely, the transformation first rotates through the angle 
theta (in radians, counterclockwise), and then translates by the vector t. */
void mat33Isometry(float theta, const float t[2], float isom[3][3]) {
    isom[0][0] = cos(theta);
    isom[0][1] = (-1)*sin(theta);
    isom[0][2] = t[0];
    
    isom[1][0] = sin(theta);
    isom[1][1] = cos(theta);
    isom[1][2] = t[1];

    isom[2][0] = 0;
    isom[2][1] = 0;
    isom[2][2] = 1;
}

/* Given a length-1 3D vector axis and an angle theta (in radians), builds the 
rotation matrix for the rotation about that axis through that angle. */
void mat33AngleAxisRotation(
        float theta, const float axis[3], float rot[3][3]) {
    float matrix[3][3] = {{       0, -axis[2],  axis[1]},
                           { axis[2],        0, -axis[0]},
                           {-axis[1],  axis[0],        0}};
                    
    float matrixSquared[3][3] = {{    pow(axis[0], 2) - 1,  axis[0] * axis[1], axis[0] * axis[2]},
                                  {axis[0] * axis[1],      pow(axis[1], 2) - 1, axis[1] * axis[2]},
                                  {axis[0] * axis[2],  axis[1] * axis[2],     pow(axis[2], 2) - 1}};
    
    float identity[3][3] = {{1, 0, 0},
                             {0, 1, 0},
                             {0, 0, 1}};
    
    // M = I + (sin(theta) * U) + ((1-cos(theta)) * U^2)
    for (int i = 0 ; i < 3 ; i++) {
        for (int j = 0 ; j < 3 ; j++) {
            rot[i][j] = identity[i][j] + sin(theta) * matrix[i][j] + (1 - cos(theta)) * matrixSquared [i][j];
        }
    }
}

/* Given two length-1 3D vectors u, v that are perpendicular to each other. 
Given two length-1 3D vectors a, b that are perpendicular to each other. Builds 
the rotation matrix that rotates u to a and v to b. */
void mat33BasisRotation(
        const float u[3], const float v[3], const float a[3], 
        const float b[3], float rot[3][3]) {
    float aCrossB[3], uCrossV[3];
    vec3Cross(a, b, aCrossB);
    vec3Cross(u, v, uCrossV);

    float r[3][3], s[3][3], rTranspose[3][3];
    for (int i = 0 ; i < 3 ; i++) {
        r[i][0] = u[i];
        r[i][1] = v[i];
        r[i][2] = uCrossV[i];
        s[i][0] = a[i];
        s[i][1] = b[i];
        s[i][2] = aCrossB[i];
    }

    mat33Transpose(r, rTranspose);
    mat333Multiply(s, rTranspose, rot);
}

/* Computes the transpose M^T of the given 4x4 matrix M. The output CANNOT safely 
alias the input. */
void mat44Transpose(const float m[4][4], float mT[4][4]) {
    for (int i = 0 ; i < 4 ; i += 1)
        for (int j = 0 ; j < 4 ; j += 1)
            mT[i][j] = m[j][i];
}

/* Multiplies m by n, placing the answer in mTimesN. The output CANNOT safely 
alias the input. */
void mat444Multiply(
        const float m[4][4], const float n[4][4], float mTimesN[4][4]) {
    for (int i = 0 ; i < 4 ; i++) {
        for (int j = 0 ; j < 4 ; j++) {
            mTimesN[i][j] = 0;
            for (int k = 0 ; k < 4 ; k++) {
                mTimesN[i][j] += m[i][k] * n[k][j];
            }
        }
    }
}

/* Multiplies m by v, placing the answer in mTimesV. The output CANNOT safely 
alias the input. */
void mat441Multiply(
        const float m[4][4], const float v[4], float mTimesV[4]) {
    mTimesV[0] = m[0][0]*v[0] + m[0][1]*v[1] + m[0][2]*v[2] + m[0][3]*v[3];
    mTimesV[1] = m[1][0]*v[0] + m[1][1]*v[1] + m[1][2]*v[2] + m[1][3]*v[3];
    mTimesV[2] = m[2][0]*v[0] + m[2][1]*v[1] + m[2][2]*v[2] + m[2][3]*v[3];
    mTimesV[3] = m[3][0]*v[0] + m[3][1]*v[1] + m[3][2]*v[2] + m[3][3]*v[3];
}

/* Given a rotation and a translation, forms the 4x4 homogeneous matrix 
representing the rotation followed in time by the translation. */
void mat44Isometry(
        const float rot[3][3], const float trans[3], float isom[4][4]) {
    for (int i = 0 ; i < 3 ; i++) {
        for (int j = 0 ; j < 3 ; j++) {
            isom[i][j] = rot[i][j];
        }
        isom[i][3] = trans[i];
    }
    isom[3][0] = 0;
    isom[3][1] = 0;
    isom[3][2] = 0;
    isom[3][3] = 1;
}


/* Sets its argument to the 4x4 zero matrix (which consists entirely of 0s). */
void mat44Zero(float m[4][4]) {
    for (int i = 0 ; i < 4 ; i++) {
        for (int j = 0 ; j < 4 ; j++) {
            m[i][j] = 0;
        }
    }
}

/* Multiplies the transpose of the 3x3 matrix m by the 3x1 matrix v. To 
clarify, in math notation it computes M^T v. The output CANNOT safely alias the 
input. */
void mat331TransposeMultiply(
        const float m[3][3], const float v[3], float mTTimesV[3]) {
    mTTimesV[0] = m[0][0]*v[0] + m[1][0]*v[1] + m[2][0]*v[2];
    mTTimesV[1] = m[0][1]*v[0] + m[1][1]*v[1] + m[2][1]*v[2];
    mTTimesV[2] = m[0][2]*v[0] + m[1][2]*v[1] + m[2][2]*v[2];
}

/* Builds a 4x4 matrix for a viewport with lower left (0, 0) and upper right 
(width, height). This matrix maps a projected viewing volume 
[-1, 1] x [-1, 1] x [-1, 1] to screen [0, w] x [0, h] x [0, 1] (each interval 
in that order). */
void mat44Viewport(float width, float height, float view[4][4]) {
    mat44Zero(view);
    view[0][0] = width / 2.0;
    view[0][3] = width / 2.0;
    view[1][1] = height / 2.0;
    view[1][3] = height / 2.0;
    view[2][2] = 0.5;
    view[2][3] = 0.5;
    view[3][3] = 1;
}

/* Inverse to the matrix produced by mat44Viewport. */
void mat44InverseViewport(float width, float height, float view[4][4]) {
    mat44Zero(view);
    view[0][0] = 2.0 / width;
    view[0][3] = -1.0;
    view[1][1] = 2.0 / height;
    view[1][3] = -1.0;
    view[2][2] = 2.0;
    view[2][3] = -1.0;
    view[3][3] = -1;
}