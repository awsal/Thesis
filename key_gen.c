/**********************************************************************
 *                                                                    *
 * Created by Adam Brockett                                           *
 *                                                                    *
 * Copyright (c) 2010                                                 *
 *                                                                    *
 * Redistribution and use in source and binary forms, with or without *
 * modification is allowed.                                           *
 *                                                                    *
 * But if you let me know you're using my code, that would be freaking*
 * sweet.                                                             *
 * Modified by Aws Al-Baldawi as part of master's thesis at LTH 2024. *
 **********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gmp.h>
#include <time.h>



#define MODULUS_SIZE 2048             /* This is the number of bits we want in the modulus, this define the size of n */
#define BUFFER_SIZE ((MODULUS_SIZE/8) / 2)  /* This is the number of bytes in p and q, less bytes will equal smaller p & q */


typedef struct {
    mpz_t n; /* Modulus */
    mpz_t e; /* Public Exponent */
} public_key;

typedef struct {
    mpz_t n; /* Modulus */
    mpz_t e; /* Public Exponent */
    mpz_t d; /* Private Exponent */
    mpz_t p; /* Starting prime p */
    mpz_t q; /* Starting prime q */
} private_key;

/* Function to print a byte array in hexadecimal format */
void print_hex(char* arr, int len)
{
    int i;
    for(i = 0; i < len; i++)
        printf("%02x", (unsigned char) arr[i]); 
}


/* Generate RSA keys : private ( ku ) and public ( kp ) */
/* NOTE: Assumes mpz_t's are initted in ku and kp */

void generate_keys(private_key* ku, public_key* kp)
{
    char buf[BUFFER_SIZE];
    int i;
    mpz_t phi; mpz_init(phi);
    mpz_t tmp1; mpz_init(tmp1);
    mpz_t tmp2; mpz_init(tmp2);
    

    /* Insetead of selecting e st. gcd(phi, e) = 1; 1 < e < phi, lets choose e
       first then pick p,q st. gcd(e, p-1) = gcd(e, q-1) = 1
        Use higher primes will enhace the performance */
    mpz_set_ui(ku->e, 65537); 

    /* Select p and q */
    /* Start with p */
    for(i = 0; i < BUFFER_SIZE; i++)
        buf[i] = rand() % 0xFF; 
    buf[0] |= 0xC0;  // Ensure int(tmp) is relatively large
    buf[BUFFER_SIZE - 1] |= 0x01;  // Ensure int(tmp) is odd
    mpz_import(tmp1, BUFFER_SIZE, 1, sizeof(buf[0]), 0, 0, buf);
    mpz_nextprime(ku->p, tmp1);

    /* Ensure p is coprime with e ( gcd (p -1 , e ) == 1) */
    mpz_mod(tmp2, ku->p, ku->e);
    while(!mpz_cmp_ui(tmp2, 1)) {
        mpz_nextprime(ku->p, ku->p);
        mpz_mod(tmp2, ku->p, ku->e);
    }

    /* Select prime q , distinct from p */
    do {
        for(i = 0; i < BUFFER_SIZE; i++)
            buf[i] = rand() % 0xFF; 
        buf[0] |= 0xC0;
        buf[BUFFER_SIZE - 1] |= 0x01;
        mpz_import(tmp1, (BUFFER_SIZE), 1, sizeof(buf[0]), 0, 0, buf);
        mpz_nextprime(ku->q, tmp1);
        mpz_mod(tmp2, ku->q, ku->e);
        while(!mpz_cmp_ui(tmp2, 1)) {
            mpz_nextprime(ku->q, ku->q);
            mpz_mod(tmp2, ku->q, ku->e);
        }
    } while(mpz_cmp(ku->p, ku->q) == 0);

    /* Calculate n = p x q */
    mpz_mul(ku->n, ku->p, ku->q);

    /* Compute phi(n) = (p-1)(q-1) */
    mpz_sub_ui(tmp1, ku->p, 1);
    mpz_sub_ui(tmp2, ku->q, 1);
    mpz_mul(phi, tmp1, tmp2);

    /* Calculate d (multiplicative inverse of e mod phi) */
    if(mpz_invert(ku->d, ku->e, phi) == 0) {
        mpz_gcd(tmp1, ku->e, phi);
        printf("gcd(e, phi) = [%s]\n", mpz_get_str(NULL, 16, tmp1));
        printf("Invert failed\n");
    }

    /* Set public key */
    mpz_set(kp->e, ku->e);
    mpz_set(kp->n, ku->n);

    return;
}


int main()
{
    /* The number of keys to be generated */
    int no_of_keys = 100;

    /* All the following arrays can be retrieved as or printed into a text file if needed */
    /* Number of hex digit for 2048 bits is (2048 / 4) + 1, each hex digit is 4 bits and 1 as the 0 terminator */

    char public_keys[no_of_keys][513];    
    char private_keys_p[no_of_keys][257];
    char private_keys_q[no_of_keys][257];
    

    

    private_key ku;
    public_key kp;
    

    /* Seed the random number generator once, otherwise it may no generate different prime numbers.
    Note that we use time () and not clock () to ensure the compatibility with Wasm */
    srand(time(NULL));
    

    struct timespec start, end;
    double tot_time = 0;

    /*FILE *file = fopen("rsa_keys.txt", "w");
    if (!file) {
        perror("Failed to open file");
        return 1;
    }*/

    for (int i = 0; i < no_of_keys; i++) {

       
	// Initialize public and private keys
        mpz_init(kp.n);
        mpz_init(kp.e);
        mpz_init(ku.n);
        mpz_init(ku.e);
        mpz_init(ku.d);
        mpz_init(ku.p);
        mpz_init(ku.q);
        // Get the current time
        clock_gettime(CLOCK_MONOTONIC, &start);

        // Generate RSA keys
        generate_keys(&ku, &kp);

        // Get the current time again
        clock_gettime(CLOCK_MONOTONIC, &end);

        // Calculate elapsed time in seconds
        double time_spent = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        tot_time += time_spent;

        // Saving the generated public and private keys
        strcpy(public_keys[i], mpz_get_str(NULL, 16, kp.n));
        strcpy(private_keys_p[i], mpz_get_str(NULL, 16, ku.p));
        strcpy(private_keys_q[i], mpz_get_str(NULL, 16, ku.q));
        
	
	/* Saving the rsults inot a text file. */
	//fprintf(file, "Key Pair %d:\n", i + 1);
        //fprintf(file, "Public Key (n): 0x%s\n", public_keys[i]);
        //fprintf(file, "Private Key (p): 0x%s\n", private_keys_p[i]);
        //fprintf(file, "Private Key (q): 0x%s\n", private_keys_q[i]);
        
	// Clear the memory allocated for keys
        mpz_clear(kp.n);
        mpz_clear(kp.e);
        mpz_clear(ku.n);
        mpz_clear(ku.e);
        mpz_clear(ku.d);
        mpz_clear(ku.p);
        mpz_clear(ku.q);
    }

    /*fprintf(file, "Time taken to generate %d RSA keys: %.9f seconds\n", no_of_keys, tot_time);
    fclose(file);*/

    printf("Time taken to generate %d RSA keys: %.9f seconds\n", no_of_keys, tot_time);

    // Print generated RSA keys
   // for (int i = 0; i < no_of_keys; i++) {
   //     printf("Key Pair %d:\n", i + 1);
   //     printf("Public Key (n): 0x%s\n", public_keys[i]);
   //     printf("Private Key (p): 0x%s\n", private_keys_p[i]);
   //     printf("Private Key (q): 0x%s\n", private_keys_q[i]);
   //     printf("\n");
   // }

    

    return 0;

}
