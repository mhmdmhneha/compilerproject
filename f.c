// THIS IS A SIMPLE EXAMPLE THAT PASSES PROG13 WITH -O0 BUT FAILS WITH -03
// SUPPLIED FOR DEBUGGING CONTROL FLOW.

int A[100];
int ff(int k, int *v) {
int B[100];
int j;
int y;
     y=k;
     for (j = 0; j < k; j++) 
           y=A[y]*B[j];
     return(y);
}
int ggg(int x) {return(A[X]); }

void main()
{ int x;
  x=ff(3,4);
  x+= ggg(x);
}

