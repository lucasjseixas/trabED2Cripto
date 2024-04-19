#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "openssl/crypto.h" //arquivo de definição necessário para SHA256_DIGEST_LENGTH
#include "openssl/sha.h" //arquivo de definição necessário função SHA256
#include "mtwister.h"

typedef struct BlocoNaoMinerado{
unsigned int numero;
unsigned int nonce;
unsigned char data[184];
unsigned char hashAnterior[SHA256_DIGEST_LENGTH];
}BlocoNaoMinerado;

typedef struct BlocoMinerado{
  BlocoNaoMinerado bloco;
  unsigned char hash[SHA256_DIGEST_LENGTH];
}BlocoMinerado;

typedef struct cabeca{
  int qntcarteiras;
  struct nolista *prim;
}cabeca;

typedef struct nolista{
  unsigned char nrocarteira;
  unsigned int qtd;
  struct nolista *prox;
  }nolista;

typedef struct listahash{
  unsigned char hash[SHA256_DIGEST_LENGTH];
  int qtd;
  struct listahash *prox;
}listahash;

typedef struct noarvore{
  int qnttransacoes;
  int nrobloco;
  struct noarvore *esq, *dir;
}noarvore;

typedef struct intchar{
  int bloco;
  unsigned char carteira;
}intchar;

typedef struct intint{
  int bloco, nonce;
}intint;

void preencherComZeroInt(unsigned int vetor[], int n);
void preencherComZeroChar(unsigned char vetor[], int n);
void preencherComZeroStructHash(listahash vetor[], int n);
void adicionaLista(cabeca *head, nolista **pp, int n);
void removeLista(cabeca *head, nolista **pp, int n);
void imprimelista(nolista *p, unsigned int carteira[]);
void printHash(unsigned char hash[], int length);
void resetHex(unsigned char vetor[], int n);
void atualizaLista(cabeca *p, nolista *no, unsigned int carteira[]);
int encontraLista(nolista *pp, int n);
void apagalista(nolista *p);
void imprimemaior(nolista *p, int i);
void inseremaior(nolista **p, unsigned int carteira[]);
void guardanumerotransacoes(unsigned char *maior, unsigned char *menor, unsigned char nrotransacoes, int k, listahash **maistransacoes, listahash **menostransacoes, unsigned char hash[]);
void removehash(listahash **pp);
void imprimehashnumerotransacoes(listahash *lista);
int calculanumerodetransacoes(BlocoMinerado vetor[], int i);
void emOrdem(noarvore *r, FILE *arqbin);
noarvore *inserearvore(noarvore **r, int qtdtransacoes, int i);
void imprimebloco(int n, FILE *arqbin);
void arquivoIndice(FILE *arqind, FILE *arqbin);
void arquivoNonce(FILE *arqnon, FILE *arqbin);
void imprimeBlocoMinEnd(FILE *arqind, FILE *arqbin);
void imprimeBlocoNonce(FILE *arqnon, FILE *arqbin);

void main(){

  //cria a "carteira" auxiliar que guarda o endereco de todas as carteiras que possuem bitcoins
  cabeca *lista = malloc(sizeof(cabeca));
  lista->qntcarteiras = 0;
  lista->prim = NULL;

  float media = 0;
  long int soma = 0;
  int nrousuario;
  int buffer[1024];

  nolista *maisbitcoins = NULL;
  nolista *maisminerou = NULL;
  listahash *maistransacoes = NULL;
  listahash *menostransacoes = NULL;
  noarvore *arvore = NULL;

  //cria a carteira e preenche com zero
  unsigned int carteira[256], qtdminerou[256]; //vetor qtdminerou sera incrementado em 1 na posicao que eh o endereco que minerou o bloco
  preencherComZeroInt(carteira, 256);          //para guardar a informacao de quantos blocos cada endereco minerou  
  preencherComZeroInt(qtdminerou, 256);

  //cria uma variavel para guardar o hash resultado da mineracao
  unsigned char hashRelevante[SHA256_DIGEST_LENGTH];

  //cria o vetor onde serao salvos os blocos 
  BlocoMinerado mandarArquivo[16];

  //cria as files 
  FILE *arqtxt = NULL;
  arqtxt = fopen("arquivo.txt", "w");
  assert(arqtxt);

  FILE *arqbin = NULL;
  arqbin = fopen("arquivo.bin", "wb");
  assert(arqbin);

  FILE *arqind = NULL;
  arqind = fopen("indice_endereco.bin", "wb");
  assert(arqind);

  FILE *arqnon = NULL;

  //escolha da seed a ser usada no gerador aleatorio
  MTRand r = seedRand(1234567);
  unsigned char valorRand = 0;

  //preencher o bloco genesis
  char frase[] = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
  BlocoNaoMinerado genesis;
  genesis.numero = 1;
  genesis.nonce = 0;
  preencherComZeroChar(genesis.data, 184);
  strcpy(genesis.data, frase);
  preencherComZeroChar(genesis.hashAnterior, SHA256_DIGEST_LENGTH);

  //escolhe a carteira que vai receber os 50 bitcoins pela mineracao do genesis
  valorRand = (unsigned char)(genRandLong(&r) % 256);
  genesis.data[183] = valorRand;  

  //cria o vetor que armazena o hash
  unsigned char hash[SHA256_DIGEST_LENGTH];

  //mineracao do bloco genesis
  while (hash[0] != 0){
    SHA256((unsigned char *)&genesis, sizeof(genesis), hash);
      if (hash[0] != 0)
        genesis.nonce++;
    }


    carteira[valorRand] = 50;
    qtdminerou[valorRand] += 1; 
    mandarArquivo[0].bloco = genesis;
    adicionaLista(lista, &(lista->prim), valorRand);

    buffer[0] = 1;
    buffer[1] = 32;
    buffer[2] = valorRand;
    buffer[3] = 10;

    //salva o hash e guarda ele para ser usado depois
    memcpy(mandarArquivo[0].hash, hash, SHA256_DIGEST_LENGTH);
    memcpy(hashRelevante, mandarArquivo[0].hash, SHA256_DIGEST_LENGTH);

  int i = 1;
  int k = 2;

  unsigned char maior;
  unsigned char menor;

  while(k < 30000){ // k sendo o numero de blocos 
    while(i < 16){

      //reseta o hash
      resetHex(hash, SHA256_DIGEST_LENGTH);
        
      //escolhe o numero de transacoes a serem feitas
      valorRand = (unsigned char)(genRandLong(&r) % 62);
      //printf("\nnumero de transações: %d\n", valorRand);

      //preenche os campos do bloco
      mandarArquivo[i].bloco.numero = k;
      mandarArquivo[i].bloco.nonce = 0;
      preencherComZeroChar(mandarArquivo[i].bloco.data, 184);
      memcpy(mandarArquivo[i].bloco.hashAnterior, hashRelevante, SHA256_DIGEST_LENGTH);

      //preenche o campo data do bloco com as transacoes
      for(int z = 0; z < (valorRand * 3); z = z + 3){
        mandarArquivo[i].bloco.data[z] = encontraLista(lista->prim, (unsigned char)(genRandLong(&r) % lista->qntcarteiras));
        mandarArquivo[i].bloco.data[z + 1] = (unsigned char)(genRandLong(&r) % 256);
        if (carteira[(int )mandarArquivo[i].bloco.data[z]] == 0)
          mandarArquivo[i].bloco.data[z + 2] = 0;
        else
          mandarArquivo[i].bloco.data[z + 2] = (unsigned char)(genRandLong(&r) % carteira[(int )mandarArquivo[i].bloco.data[z]]+1);
        carteira[(int )mandarArquivo[i].bloco.data[z]] -= mandarArquivo[i].bloco.data[z + 2];
        //printf("\n%d %d %d\n",mandarArquivo[i].bloco.data[z], mandarArquivo[i].bloco.data[z + 1], mandarArquivo[i].bloco.data[z + 2]);

        soma += mandarArquivo[i].bloco.data[z + 2];

      }

      //escolher quem ganha os 50 bitcoins
      unsigned char minerou = (unsigned char) (genRandLong(&r) % 256);
      mandarArquivo[i].bloco.data[183] = minerou;
      

      //minerar o bloco
      while (hash[0] != 0){
      SHA256((unsigned char *)&mandarArquivo[i].bloco, sizeof(mandarArquivo[i].bloco), hash);
        mandarArquivo[i].bloco.nonce++;
      }
      memcpy(mandarArquivo[i].hash, hash, SHA256_DIGEST_LENGTH);
      memcpy(hashRelevante, mandarArquivo[i].hash, SHA256_DIGEST_LENGTH);
      guardanumerotransacoes(&maior, &menor, valorRand, k, &maistransacoes, &menostransacoes, hash);

      //da os 50 btc p/ cara que minerou
      carteira[minerou] += 50;
      qtdminerou[minerou] += 1;
      adicionaLista(lista, &(lista->prim), minerou);

      for(int z = 0; z < (valorRand * 3); z += 3)
      {
        carteira[mandarArquivo[i].bloco.data[z + 1]] += mandarArquivo[i].bloco.data[z + 2];
        if((carteira[mandarArquivo[i].bloco.data[z + 1]]) > 0)
        adicionaLista(lista, &(lista->prim), mandarArquivo[i].bloco.data[z + 1]);
      }
      atualizaLista(lista, lista->prim, carteira);
      k++;
      i++;
    }
    i = 0;

    while(i < 16){
      //escreve na file .txt
      fprintf(arqtxt, "%d\n", mandarArquivo[i].bloco.numero);
      fprintf(arqtxt, "%d\n", mandarArquivo[i].bloco.nonce);
      fprintf(arqtxt, "%d\n", mandarArquivo[i].bloco.data[183]);

      for(int j = 0; j < SHA256_DIGEST_LENGTH; j++){
        fprintf(arqtxt, "%02x", mandarArquivo[i].bloco.hashAnterior[j]);
      }
      fprintf(arqtxt, "%s", "\n");
      for(int j = 0; j < SHA256_DIGEST_LENGTH; j++){
        fprintf(arqtxt, "%02x", mandarArquivo[i].hash[j]);
      }
      fprintf(arqtxt, "%s", "\n\n");

      i++;
    }
  i = 0;
  //escreve os 16 blocos no arquivo .bin
  fwrite(mandarArquivo, sizeof(BlocoMinerado), 16, arqbin);
  }

  // for(int z = 0; z < 256; z++){
  //   if(carteira[z] > 0)
  //     printf("\na carteira %d minerou %d vezes", z, qtdminerou[z]);
  // }

  // media = mediaVetor(carteira, 256);
  // printf("\nMedia de BTC = %f", media);

  media = soma / 30000;

  fclose(arqtxt);
  fclose(arqbin);

//  printf("\n\n  %ld  \n\n", sizeof(intchar));
  arquivoIndice(arqind, arqbin);
  arquivoNonce(arqnon, arqbin);
  int menu = 1;
  //menu
  while(menu != 0){
  printf("\n1 - endereco com mais bitcoins e o numero de bitcoins dele\n2 - endereco que minerou mais blocos\n3 - hash do bloco com mais transacoes e o numero de transacoes dele\n4 - hash do bloco com menos transacoes e o numero de transacoes dele\n5 - quantidade media de bitcoins por bloco\n6 - imprimir todos os campos de um bloco dado o numero do bloco\n7 - imprimir todos os campos dos n primeiros blocos minerados por um dado endereco\n8 - imprimir todos os campos dos n primeiros blocos em ordem crescente de quantidade de transacoes do bloco\n9 - imprimir todos os campos de todos os blocos que tem um dado nonce\n0 - Sair\n\n");
  scanf("%d", &menu);
  switch (menu){

  case 1:
    inseremaior(&maisbitcoins, carteira); //funcao que cria uma lista cujo conteudo sempre eh a(s) carteira(s) com mais btc
    imprimemaior(maisbitcoins, 0);
    break;

  case 2:
    inseremaior(&maisminerou, qtdminerou); //funcao que cria uma lista cujo conteudo sempre eh a(s) carteira(s) que mineraram mais
    imprimemaior(maisminerou, 1);
    break;

  case 3:
    printf("o(s) hash(s) ");
    imprimehashnumerotransacoes(maistransacoes);
    break;

  case 4:
    printf("o(s) hash(s) ");
    imprimehashnumerotransacoes(menostransacoes);
    break;
  
  case 5:
    printf("\nA media de bitcoins por bloco e: %.02f\n", media);
    break;

  case 6:
    printf("Digite o numero do bloco:");
    scanf("%d", &nrousuario);
    arqbin = fopen("arquivo.bin","rb");
    imprimebloco(nrousuario, arqbin);
    fclose(arqbin);
    break;

  case 7:
    imprimeBlocoMinEnd(arqind, arqbin);
    break;

  case 8:
    int nrotransacoes, contador = 0;
    arqbin = fopen("arquivo.bin","rb");

    printf("escolha o numero de blocos:");
    scanf("%d", &nrousuario);
    //calculo de quantas vezes 
    for(int h = 1; h <= nrousuario; h++){
      if(contador == 0)
        fread(mandarArquivo, sizeof(BlocoMinerado), 16, arqbin);
      nrotransacoes = calculanumerodetransacoes(mandarArquivo, contador);
      inserearvore(&arvore, nrotransacoes, h);
      contador++;
      contador = contador % 16;
    }

    emOrdem(arvore, arqbin);
    fclose(arqbin);
    break;

  case 9:
    imprimeBlocoNonce(arqnon, arqbin);
    break;

  case 0:
    break;
  }
  }

  fclose(arqind);
  return;
};


void arquivoIndice(FILE *arqind, FILE *arqbin)
{
  arqbin = fopen("arquivo.bin","rb");
  arqind = fopen("indice_endereco.bin","wb+"); 
  intchar vetor[512];
  BlocoMinerado mandarArquivo[16];

  int indice = 0, blocosimpressos = 0;
  while(1){
    while(indice < 512){
      fread(mandarArquivo, sizeof(BlocoMinerado), 16, arqbin);
      for (int i = 0; i < 16; i++){
        vetor[indice].bloco = mandarArquivo[i].bloco.numero;
        vetor[indice].carteira = mandarArquivo[i].bloco.data[183];
        indice++;
      }
    }
    indice = 0;
    if(blocosimpressos == 29696){
      fwrite(vetor, sizeof(intchar), 304 , arqind);
      blocosimpressos+=304;
    }
    else{
      fwrite(vetor, sizeof(intchar), 512 , arqind);
      blocosimpressos+=512;
    }
    if(blocosimpressos == 30000) break;
  }
  fclose(arqbin);
  fclose(arqind);
}

void arquivoNonce(FILE *arqnon, FILE *arqbin)
{
  arqbin = fopen("arquivo.bin","rb");
  arqnon = fopen("indice_nonce.bin","wb+"); 
  intint vetor[512];
  BlocoMinerado mandarArquivo[16];
  int j = 512, blocosimpressos =0;

  int indice = 0;
  while(1){
    while(indice < 512){
      j = fread(mandarArquivo, sizeof(BlocoMinerado), 16, arqbin);
      for (int i = 0; i < 16; i++){
        vetor[indice].bloco = mandarArquivo[i].bloco.numero;
        vetor[indice].nonce = mandarArquivo[i].bloco.nonce;
        indice++;
      }
    }
    indice = 0;
    if(blocosimpressos == 29696){
      fwrite(vetor, sizeof(intint), 304 , arqnon);
      blocosimpressos+=304;
    }
    else{
      fwrite(vetor, sizeof(intint), 512 , arqnon);
      blocosimpressos+=512;
    }
    if(blocosimpressos == 30000) break;
  }

  fclose(arqbin);
  fclose(arqnon);
}

void preencherComZeroInt(unsigned int vetor[], int n){
  for(int i = 0; i < n; i++){
    vetor[i] = 0;
  }
}

void preencherComZeroChar(unsigned char vetor[], int n){
  for(int i = 0; i < n; i++){
    vetor[i] = 0;
  }
}


void adicionaLista(cabeca *head, nolista **pp, int n){
  if((*pp) == NULL){
    (*pp) = (nolista *) malloc(sizeof(nolista));
    (head->qntcarteiras)++;
    (*pp)->nrocarteira = n;
    (*pp)->prox = NULL;
  }
  else{
    //retorna caso a carteira ja esteja na lista
    if((*pp)->nrocarteira == n){
      return;
    }
    //chamada recursiva
    adicionaLista(head, (&(*pp)->prox), n);
  }
}

void removeLista(cabeca *head, nolista **pp, int n){
  if(*pp){
    if((*pp)->nrocarteira == n){
      nolista *aux = *pp;
      *pp = (*pp)->prox;
      free(aux);
      (head->qntcarteiras)--;
    }
    else
      removeLista(head, &((*pp)->prox), n);
  }
}

void removehash(listahash **pp){
  if(*pp){
    listahash *aux = *pp;
    *pp = (*pp)->prox;
    free(aux);
    removehash(pp);
  }
  else return;
}

void imprimelista(nolista *p, unsigned int carteira[]){
  if(p != NULL){
    printf("\na carteira %d tem %d btc", p->nrocarteira, carteira[p->nrocarteira]);
    imprimelista(p->prox, carteira);
  }
}

void printHash(unsigned char hash[], int length){
  int i;
  for(i=0;i<length;++i)
    printf("%02x", hash[i]);

  printf("\n");
}

void resetHex(unsigned char vetor[], int n){
  for (int i = 0; i < n; i++) {
    vetor[i] = 0xFF;
  }
}

int encontraLista(nolista *pp, int n){
    for(int i = 0; i < n; i++){
        pp = pp->prox;
    }
    return pp->nrocarteira;
}

void atualizaLista(cabeca *p, nolista *no, unsigned int carteira[]){
  if(no == NULL) return;

  atualizaLista(p, no->prox, carteira);

  if (carteira[no->nrocarteira] == 0)
    removeLista(p, &(p->prim), no->nrocarteira);
}

void inseremaior(nolista **p, unsigned int vetor[]){
  unsigned int maior = vetor[0];
  for(int i = 0; i < 256; i++){
    if(vetor[i] > maior){
      maior = vetor[i];
      //printf("maior == %d \n", maior);
    }
  }

  for(int i = 0; i < 256; i++){
    if(vetor[i] == maior){
      if(*p == NULL){
            *p = malloc(sizeof(p));
            (*p)->nrocarteira = i;
            (*p)->qtd = maior;
            (*p)->prox = NULL;
      }
      else if(*p){
        nolista *aux;
        aux = malloc(sizeof(aux));
        aux->nrocarteira = i;
        aux->qtd = maior;
        aux->prox = *p;
        *p = aux;
      }
    }
  }
}

void imprimemaior(nolista *p, int i){
  if(i == 0)
    printf("o(s) endereco(s) com mais bitcoins eh(sao)");
  else if(i == 1)
    printf("o(s) endereco(s) que minerou mais blocos eh(sao)");
  if(!p) return;
  unsigned int aux = p->qtd;
  while(p){
    printf("%d ", p->nrocarteira);
    p = p->prox;
  }
  if(i == 0)
    printf("e eles possuem %d bitcoins\n\n", aux);
  else if(i == 1)
    printf("e eles mineraram %d vezes\n\n", aux);
  
}

void guardanumerotransacoes(unsigned char *maior, unsigned char *menor, unsigned char nrotransacoes, int k, listahash **maistransacoes, listahash **menostransacoes, unsigned char hash[]){
  
  if(k == 2){
    *maior = nrotransacoes;
    *menor = nrotransacoes;
  }
  else if (*maior < nrotransacoes){
    *maior = nrotransacoes;
    //printf("\n\n\n maior numero de transacoes %d", *maior);
    removehash(maistransacoes);
    *maistransacoes = malloc (sizeof(listahash));
    (*maistransacoes)->prox = NULL;
    memcpy((*maistransacoes)->hash, hash, SHA256_DIGEST_LENGTH);
    (*maistransacoes)->qtd = nrotransacoes;
    //imprimehashnumerotransacoes(*maistransacoes);
  }
  else if (*maior == nrotransacoes){
    listahash *aux;
    aux = malloc(sizeof(listahash));
    memcpy(aux->hash, hash, SHA256_DIGEST_LENGTH);
    aux->qtd = nrotransacoes;
    aux->prox = *maistransacoes;
    *maistransacoes = aux;
    //imprimehashnumerotransacoes(*maistransacoes);
  }
  else if (*menor > nrotransacoes){
    *menor = nrotransacoes;
    //printf("\n\n\n menor numero de transacoes %d", *menor);
    removehash(menostransacoes);
    *menostransacoes = malloc (sizeof(listahash));
    (*menostransacoes)->prox = NULL;
    memcpy((*menostransacoes)->hash, hash, SHA256_DIGEST_LENGTH);
    (*menostransacoes)->qtd = nrotransacoes;
    //imprimehashnumerotransacoes(*menostransacoes);
  }
  else if (*menor == nrotransacoes){
    listahash *aux;
    aux = malloc(sizeof(listahash));
    memcpy(aux->hash, hash, SHA256_DIGEST_LENGTH);
    aux->qtd = nrotransacoes;
    aux->prox = *menostransacoes;
    *menostransacoes = aux;
    //imprimehashnumerotransacoes(*menostransacoes);
  }
  
  
  return;
}

void imprimehashnumerotransacoes(listahash *lista){
  if (lista == NULL) return;
  printHash(lista->hash, SHA256_DIGEST_LENGTH);
  if(lista->prox == NULL)
    printf("possuem %d transacoes\n", lista->qtd);
  imprimehashnumerotransacoes(lista->prox);
}

int calculanumerodetransacoes(BlocoMinerado vetor[], int i){
  int count = 0;
  for(int k = 0; k < 183; k = k + 3){
    if(vetor[i].bloco.data[k] > 0 || vetor[i].bloco.data[k+1] > 0)
      count++;
  }
  return count;
}

noarvore *inserearvore(noarvore **r, int qtdtransacoes, int i){
  if (*r == NULL)
  {
    *r = malloc(sizeof(noarvore));
    if (*r == NULL)
      return NULL;
    (*r)->qnttransacoes = qtdtransacoes;
    (*r)->nrobloco = i;
    (*r)->esq = NULL;
    (*r)->dir = NULL;
    return *r;
  }

  if((*r)->qnttransacoes == qtdtransacoes)
    return inserearvore(&((*r)->esq), qtdtransacoes, i);
  if(qtdtransacoes < (*r)->qnttransacoes)
     return inserearvore(&((*r)->esq), qtdtransacoes, i);
  else 
     return inserearvore(&((*r)->dir), qtdtransacoes, i);
}

void emOrdem(noarvore *r, FILE *arqbin){
  if (r != NULL)
  { 
    BlocoMinerado mandarArquivo[1];
    emOrdem(r->esq, arqbin);
    printf("\n--------------------------------------\n\nquantidade de transações: %d\n", r->qnttransacoes);
      fseek(arqbin, ((r->nrobloco)-1)*sizeof(BlocoMinerado), SEEK_SET );
      fread(mandarArquivo, sizeof(BlocoMinerado), 1, arqbin);
      printf("\nnumero do bloco: %d, nonce: %d\n\ndata:", mandarArquivo->bloco.numero, mandarArquivo->bloco.nonce);
      for (int z = 0; z < 183; z = z + 3){
        printf("\n%d %d %d",mandarArquivo->bloco.data[z], mandarArquivo->bloco.data[z + 1], mandarArquivo->bloco.data[z + 2]);
      }
    printf("\nminerou:%d\n\nhash anterior:", mandarArquivo->bloco.data[183]);
    printHash(mandarArquivo->bloco.hashAnterior, SHA256_DIGEST_LENGTH);
    printf("\nhash:");
    printHash(mandarArquivo->hash, SHA256_DIGEST_LENGTH);
    printf("\n");
    rewind(arqbin);
    emOrdem(r->dir, arqbin);
  }
}

void imprimebloco(int n, FILE *arqbin){
  BlocoMinerado mandarArquivo[1];
  fseek(arqbin, (n-1)*sizeof(BlocoMinerado), SEEK_SET );
  fread(mandarArquivo, sizeof(BlocoMinerado), 1, arqbin);
  printf("\nnumero do bloco: %d, nonce: %d\n\ndata:", mandarArquivo->bloco.numero, mandarArquivo->bloco.nonce);
  if(n == 1){ //genesis
    printf("%s\n", mandarArquivo->bloco.data);
    printf("\nminerou:%d\n\nhash anterior:", mandarArquivo->bloco.data[183]);
    printHash(mandarArquivo->bloco.hashAnterior, SHA256_DIGEST_LENGTH);
    printf("\nhash:");
    printHash(mandarArquivo->hash, SHA256_DIGEST_LENGTH);
    return;
  }
  for (int z = 0; z < 183; z = z + 3){
    printf("\n%d %d %d",mandarArquivo->bloco.data[z], mandarArquivo->bloco.data[z + 1], mandarArquivo->bloco.data[z + 2]);
  }
    printf("\nminerou:%d\n\nhash anterior:", mandarArquivo->bloco.data[183]);
    printHash(mandarArquivo->bloco.hashAnterior, SHA256_DIGEST_LENGTH);
    printf("\nhash:");
    printHash(mandarArquivo->hash, SHA256_DIGEST_LENGTH);
}

void imprimeBlocoMinEnd(FILE *arqind, FILE *arqbin){
  arqind = fopen("indice_endereco.bin", "rb");
  arqbin = fopen("arquivo.bin", "rb");

  int blocosuser, blocoslidos = 0, blocosencontrados = 0;
  unsigned char carteira;
  intchar vetor[512];
  BlocoMinerado mandarArquivo[1];

  printf("Numero de blocos: ");
  scanf("%d", &blocosuser);
  printf("Numero da carteira (0 a 255): ");
  scanf("%hhd", &carteira);

  while(blocoslidos < 30000){
    fread(vetor, sizeof(intchar), 512, arqind);
    blocoslidos += 512;
      for (int i = 0; i < 512; i++){
        if(vetor[i].carteira == carteira){
          fseek(arqbin, (vetor[i].bloco-1)*sizeof(BlocoMinerado), SEEK_SET);
          fread(mandarArquivo, sizeof(BlocoMinerado), 1, arqbin);
          printf("\nnumero do bloco: %d, nonce: %d\n\ndata:", mandarArquivo->bloco.numero, mandarArquivo->bloco.nonce);
          if(mandarArquivo->bloco.numero == 1){ //genesis
            printf("%s\n", mandarArquivo->bloco.data);
            printf("\nminerou:%d\n\nhash anterior:", mandarArquivo->bloco.data[183]);
            printHash(mandarArquivo->bloco.hashAnterior, SHA256_DIGEST_LENGTH);
            printf("\nhash:");
            printHash(mandarArquivo->hash, SHA256_DIGEST_LENGTH);
          }
          else {
            for (int z = 0; z < 183; z = z + 3){
              printf("\n%d %d %d",mandarArquivo->bloco.data[z], mandarArquivo->bloco.data[z + 1], mandarArquivo->bloco.data[z + 2]);
            }
          }
            printf("\nminerou:%d\n\nhash anterior:", mandarArquivo->bloco.data[183]);
            printHash(mandarArquivo->bloco.hashAnterior, SHA256_DIGEST_LENGTH);
            printf("\nhash:");
            printHash(mandarArquivo->hash, SHA256_DIGEST_LENGTH);
          blocosencontrados++;
          if(blocosencontrados == blocosuser){
            fclose(arqind);
            fclose(arqbin);
            return;
          }
        }
      }
  }
}

void imprimeBlocoNonce(FILE *arqnon, FILE *arqbin){
  arqnon = fopen("indice_nonce.bin", "rb");
  arqbin = fopen("arquivo.bin", "rb");

  unsigned int nonceuser = 0;
  int blocoslidos = 0, j;
  intint vetor[512];
  BlocoMinerado mandarArquivo[1];

  printf("Numero do nonce: ");
  scanf("%d", &nonceuser);

  while(1){
    j = fread(vetor, sizeof(intint), 512, arqnon);
    blocoslidos += 512;
      for (int i = 0; i < j; i++){
        if(vetor[i].nonce == nonceuser){
          fseek(arqbin, (vetor[i].bloco-1)*sizeof(BlocoMinerado), SEEK_SET);
          fread(mandarArquivo, sizeof(BlocoMinerado), 1, arqbin);
          printf("\nnumero do bloco: %d, nonce: %d\n\ndata:", mandarArquivo->bloco.numero, mandarArquivo->bloco.nonce);
          if(mandarArquivo->bloco.numero == 1){ //genesis
            printf("%s\n", mandarArquivo->bloco.data);
            printf("\nminerou:%d\n\nhash anterior:", mandarArquivo->bloco.data[183]);
            printHash(mandarArquivo->bloco.hashAnterior, SHA256_DIGEST_LENGTH);
            printf("\nhash:");
            printHash(mandarArquivo->hash, SHA256_DIGEST_LENGTH);
          }
          else {
            for (int z = 0; z < 183; z = z + 3){
              printf("\n%d %d %d",mandarArquivo->bloco.data[z], mandarArquivo->bloco.data[z + 1], mandarArquivo->bloco.data[z + 2]);
            }
            printf("\nminerou:%d\n\nhash anterior:", mandarArquivo->bloco.data[183]);
            printHash(mandarArquivo->bloco.hashAnterior, SHA256_DIGEST_LENGTH);
            printf("\nhash:");
            printHash(mandarArquivo->hash, SHA256_DIGEST_LENGTH);
          }
        }
    
    }
    if(blocoslidos > 30000) break;
  }

  fclose(arqnon);
  fclose(arqbin);
  return;
}