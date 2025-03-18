#include <stdio.h>
void multiplymatrices(int *A, int *B, int *C, int r1, int c1, int r2, int c2) {
    for(int i=0;i<r1;i++){
        for(int j=0;j<c2;j++){
            *(C+i*c2+j)=0;
            for(int k=0;k<c1;k++){
                *(C+i*c2+j) += *(A+i*c1+k) * *(B+k*c2+j);
            }
        }
    }
}
int main(void){
    int r1,r2,c1,c2;
    scanf("%d %d",&r1, &c1);
    int A[r1][c1];
    for(int i=0;i<r1;i++){
        for(int j=0;j<c1;j++){
            scanf("%d",&(A[i][j]));
        }
    }
    scanf("%d %d",&r2, &c2);
    int B[r1][c1];
    for(int i=0;i<r2;i++){
        for(int j=0;j<c2;j++){
            scanf("%d",&(B[i][j]));
        }
    }
    if (c1!=r2){
        printf("Invalid dimensions");
    }else{
    int C[r1][c2];
    multiplymatrices((int*)A,(int*)B,(int*)C,r1,c1,r2,c2);
    for(int i=0;i<r1;i++){
        for(int j=0;j<c2;j++){
            printf("%d ",*(C+i*c2+j));
        }
    }
    }
    return 0;
}