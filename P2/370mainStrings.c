


/* On macOS, compile with...
    clang 370mainStrings.c
On Ubuntu, compile with...
    cc 370mainStrings.c
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* From here down to main(), we construct a String pseudo-class. */
typedef struct String String;
struct String {
    unsigned int length;
    char *contents;
};

/* Constructs a String from a C string. */
String String_string(char *cString) {
    String s;
    s.length = strlen(cString);
    s.contents = (char *)malloc((s.length + 1) * sizeof(char));
    if (s.contents == NULL)
        s.length = 0;
    else
        strcpy(s.contents, cString);
    return s;
}

/* Deletes a String. This function must be used, whenever a  String is no longer 
needed, to free up its memory. */
void String_finalize(String *s) {
    if (s->contents != NULL)
        free(s->contents);
    s->contents = NULL;
}

/* Prints a String. */
void String_print(String *s) {
    if (s->contents != NULL)
        printf("%s", s->contents);
}

/* Returns the ith character of the given String. */
char String_entry(String *s, unsigned int i) {
    if (i < s->length)
        return s->contents[i];
    else
        return 0;
}

/* Returns a new String, consisting of characters i through j-1 of the old 
String. */
String String_slice(String *s, unsigned int i, unsigned int j) {
    if (i >= j)
        return String_string("");
    if (j > s->length)
        j = s->length;
    String new;
    new.length = j - i;
    new.contents = (char *)malloc((new.length + 1) * sizeof(char));
    if (new.contents == NULL)
        new.length = 0;
    else {
        strncpy(new.contents, &(s->contents[i]), j - i);
        new.contents[j] = 0;
    }
    return new;
}

/* Returns a new String, consisting of the two given Strings concatenated 
together, without altering those Strings. */
String String_concat(String *a, String *b) {
    String new;
    new.length = a->length + b->length;
    new.contents = (char *)malloc((new.length + 1) * sizeof(char));
    if (new.contents == NULL)
        new.length = 0;
    else {
        strcpy(new.contents, a->contents);
        strcpy(&(new.contents[a->length]), b->contents);
    }
    return new;
}

/* This is the main program, that runs when you execute the file. */
int main(int argc, char** argv) {
    /* The classic Hello, World. */
    String a = String_string("Hello, ");
    String b = String_string("world.");
    String c = String_concat(&a, &b);
    String_print(&c);
    String_finalize(&c);
    printf("\n");
    int numTrials = 1000000, n;
    clock_t startTime, endTime;
    /* Time how long numTrials concatenations take. */
    startTime = clock();
    for (n = 0; n < numTrials; n += 1) {
        c = String_concat(&a, &b);
        //String_print(&c);
        String_finalize(&c);
    }
    endTime = clock();
    printf("%d abstracted string concatenations take %f seconds\n", 
        numTrials, (double)(endTime - startTime) / CLOCKS_PER_SEC);
    /* Time how long they take, if you manually re-use the memory. */
    c = String_concat(&a, &b);
    startTime = clock();
    for (n = 0; n < numTrials; n += 1) {
        strcpy(c.contents, a.contents);
        strcpy(&(c.contents[a.length]), b.contents);
        //String_print(&c);
    }
    endTime = clock();
    printf("%d low-level string concatentations take %f seconds\n", 
        numTrials, (double)(endTime - startTime) / CLOCKS_PER_SEC);
    String_finalize(&c);
    /* Repeat the first test. */
    startTime = clock();
    for (n = 0; n < numTrials; n += 1) {
        c = String_concat(&a, &b);
        //String_print(&c);
        String_finalize(&c);
    }
    endTime = clock();
    printf("%d abstracted string concatenations take %f seconds\n", 
        numTrials, (double)(endTime - startTime) / CLOCKS_PER_SEC);
    /* Repeat the second test. */
    c = String_concat(&a, &b);
    startTime = clock();
    for (n = 0; n < numTrials; n += 1) {
        strcpy(c.contents, a.contents);
        strcpy(&(c.contents[a.length]), b.contents);
        //String_print(&c);
    }
    endTime = clock();
    printf("%d low-level string concatentations take %f seconds\n", 
        numTrials, (double)(endTime - startTime) / CLOCKS_PER_SEC);
    String_finalize(&c);
    /* Repeat the first test. */
    startTime = clock();
    for (n = 0; n < numTrials; n += 1) {
        c = String_concat(&a, &b);
        //String_print(&c);
        String_finalize(&c);
    }
    endTime = clock();
    printf("%d abstracted string concatenations take %f seconds\n", 
        numTrials, (double)(endTime - startTime) / CLOCKS_PER_SEC);
    /* Repeat the second test. */
    c = String_concat(&a, &b);
    startTime = clock();
    for (n = 0; n < numTrials; n += 1) {
        strcpy(c.contents, a.contents);
        strcpy(&(c.contents[a.length]), b.contents);
        //String_print(&c);
    }
    endTime = clock();
    printf("%d low-level string concatentations take %f seconds\n", 
        numTrials, (double)(endTime - startTime) / CLOCKS_PER_SEC);
    String_finalize(&c);
    /* Don't forget to clean up everything. */
    String_finalize(&a);
    String_finalize(&b);
    return 0;
}

/* Toward true object-orientation: A class is a lookup table, mapping strings to 
functions. Each object has a pointer to its class. Methods are invoked using 
syntax such as 'tell(&a, "concat", &b);'. The tell function asks a->class for 
its "concat" function. The class responds with something like String_concat. 
Then tell calls 'String_concat(&a, &b);' and returns the result. (For 
uniformity, all functions should really return _pointers_ to objects.)

To make the syntax _look_ more object-oriented: Let the user type syntax such as 
'a.concat(b);'. Run a pre-processor to rewrite this syntax as 'tell(&a, 
"concat", &b);'. Then compile the program as usual. To learn more, search the 
web for 'ooc.pdf' by Axel-Tobias Schreiner. */


