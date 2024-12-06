
int arr[10][10];

int test_1(){
    int i;
    for (int x = 0; x<10 ; x++) {
    for (int y = 0; y<10 ; y++) {
        i = arr[x][y];
    }

    }
    return i;
}

int test_2(){
    int i;
        int *ptr = &arr[1][1];
    for (int x = 1; x<10-1 ; x++) {

        for (int y = 1; y<10-1 ; y++) {
            i = *ptr++;
        }
        ptr+=2;

    }

    return i;
}

int main() {

    int i = 0;

    test_1();
    test_2();


    return i;
}