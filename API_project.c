//importazione librerie
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include<stdbool.h>
#include<stdint.h>
#include<assert.h>

//definizione costanti
#define MAX_LEN 20//20 sembra funzionare, dovrebbe essere 255
#define BASE_SIZE_RIC (uint32_t)65536
#define BASE_SIZE_MAG (uint32_t)65536
#define INFTY 2147483647
#define FNV_PRIME 0x100000001b3
#define FNV_OFFSET 0xcbf29ce48422325
#define MAG_SIZE_HEX 0xffff

//dichiarazione tipi
//hash table (magazzino)-->vettore di min-heap: ogni ingrediente identifica un min heap ordinato per scadenza+ ricettario--> vettore di liste
typedef struct LOTTO{
    int peso;
    int scadenza;
    struct LOTTO* next;
}lotto;

typedef struct HEAD_MAG{
    char key[MAX_LEN];
    int peso_tot;
    int numero; //nelle ricette rappresenta il numero di ingredienti, nel magazzino il numero di lotti
    lotto* next_same; //prossimo lotto
    struct HEAD_MAG* next_item; //prossimo ingrediente
}head_mag;

typedef struct INGREDIENTE{
    char key[MAX_LEN];
    int peso;
    struct INGREDIENTE* next;
    head_mag* nel_magazzino;
}ingrediente;

typedef struct HEAD_RIC{
    char key[MAX_LEN];
    int peso_tot;
    int numero; //nelle ricette rappresenta il numero di ingredienti, nel magazzino il numero di lotti
    int ordiniAttivi; //nelle ricette rappresentail numero di ordini da evadere
    ingrediente* next_same; //nelle ricette il prossimo ingrediente
    struct HEAD_RIC* next_item; //prossima ricetta
}head_ric;


//coda (ordini in attesa, double linked list)
typedef struct CODA{
    head_ric* p_ric; //punta alla testa della ricetta corrispondente
    int unita;
    int peso;
    int data;
    struct CODA* next;
    struct CODA* prev;
    bool isReady;
}ElemCoda;

typedef ElemCoda* coda;

typedef struct HASH_TABLE_MAG{
    uint32_t size;
    uint32_t occupied;
    head_mag** elements;
}hash_table_mag;

typedef struct HASH_TABLE_RIC{
    uint32_t size;
    uint32_t occupied;
    head_ric** elements;
}hash_table_ric;

//prototipi funzioni
//hash
uint64_t hash(char* key);
hash_table_ric* crea_ricettario(uint32_t size);
hash_table_mag* crea_magazzino(uint32_t size);
int hash_table_insert_mag(lotto* lotto, int hashed, char* key);
void inserisci_in_ordine(head_mag* testa, lotto* lotto);
int hash_table_insert_ric(char* key, int hashed);
//generiche
void rimuovi_ricetta();
void cancella_ricetta(head_ric* testa);
void rifornimento();
lotto* crea_lotto(int peso, int scadenza);
head_mag* lookup_magazzino(char* key);
head_ric* lookup_ricettario(char* key);
int gestione_ordine(char* ricetta, int unita);
coda crea_elem_coda(head_ric* p_ric, int unita, bool isReady);
void inserisci_elem_coda(coda elem);
void prepara_ordine(head_ric* ric, int unita, int* quantita_richieste);
void aggiorna_scadmin();
void verifica_magazzino();
void gestione_attesa();
void inserisci_pronti_cronologico(coda elem);
void carica_ordini();
coda* mergeSort(coda* arr, int p, int r);
coda* merge(coda* arr, int p, int q, int r);

//variabili globali
hash_table_mag* magazzino;
hash_table_ric* ricettario;
coda pronti=NULL,attese=NULL, fine_attese=NULL, fine_pronti=NULL;
int scad_min=INFTY; //inizialmente scad_min è infinita
int num_pronti=0;
int max_carico,periodo;
int t=0;


int main(){
    //dichiarazione variabili
    int i,unita,sent=0;
    char s[MAX_LEN];
    //char* casi[4]={"aggiungi_ricetta","rimuovi_ricetta","rifornimento","ordine"};
    char confronto;
    head_mag* p_mag, *temp_mag;
    head_ric* p_ric, *temp_ric;
    ingrediente* p_ingr, *temp_ingr;
    lotto* p_lot, *temp_lot;


    magazzino=crea_magazzino(BASE_SIZE_MAG);
    ricettario=crea_ricettario(BASE_SIZE_RIC);
    assert(scanf("%d %d",&periodo,&max_carico));
    assert(scanf("%s",s));
    while(!sent){
        if(t!=0&&!(t%periodo)){ //se passa il camioncino
            carica_ordini();
        }
        if(!feof(stdin)){
            if(t==scad_min){ //se qualcosa scade
                verifica_magazzino();
            }
            confronto=s[2]; //il terzo carattere del comando lo identifica!
            switch(confronto){
                case 'g':
                    assert(scanf("%s",s));
                    i=hash_table_insert_ric(s,hash(s)&MAG_SIZE_HEX);
                    if(i==-1){
                        printf("ignorato\n");
                    }
                    else{
                        printf("aggiunta\n");
                    }
                    break;
                case 'm':
                    rimuovi_ricetta();
                    break;
                case 'f':
                    rifornimento();
                    printf("rifornito\n");
                    gestione_attesa(); //verifico se posso preparare ordini in attesa
                    break;
                case 'd':
                    assert(scanf("%s %d\n",s,&unita));
                    i=gestione_ordine(s,unita);
                    if(i>=0){
                        printf("accettato\n");
                    }
                    else{ //i<0
                        printf("rifiutato\n");
                    }
                    break;
            }
            assert(scanf("%s",s));
            t++;
        }
        else{
            sent=1;
        }
    }

        //distruggo tutte le liste nei bucket, da capire se posso evitare
        for(i=0;i<magazzino->size;i++){
            p_mag=magazzino->elements[i];
            while(p_mag!=NULL){
                p_lot=p_mag->next_same;
                while(p_lot!=NULL){
                    temp_lot=p_lot;
                    p_lot=p_lot->next;
                    free(temp_lot);
                }
                temp_mag=p_mag;
                p_mag=p_mag->next_item;
                free(temp_mag);
            }
        }
        for(i=0;i<ricettario->size;i++){
            p_ric=ricettario->elements[i];
            while(p_ric!=NULL){
                p_ingr=p_ric->next_same;
                while(p_ingr!=NULL){
                    temp_ingr=p_ingr;
                    p_ingr=p_ingr->next;
                    free(temp_ingr);
                }
                temp_ric=p_ric;
                p_ric=p_ric->next_item;
                free(temp_ric);
            }
        }
    free(ricettario->elements);
    free(magazzino->elements);
    free(ricettario);
    free(magazzino);
    return 0;
}


uint64_t hash(char* key){ //hash FNV1a
    uint64_t hash = FNV_OFFSET;
    int i = 0;
    int len=strlen(key);
    for(i=0;i<len;i++){
        hash ^= key[i];
        hash *= FNV_PRIME;
    }
    return hash;
}

hash_table_ric* crea_ricettario(uint32_t size){
    hash_table_ric* ht=malloc(sizeof(*ht));
    ht->size=size;
    ht->elements=calloc(sizeof(head_ric*),size);
    ht->occupied=0;
    return ht;
}

hash_table_mag* crea_magazzino(uint32_t size){
    hash_table_mag* ht=malloc(sizeof(*ht));
    ht->size=size;
    ht->elements=calloc(sizeof(head_mag*),size);
    ht->occupied=0;
    return ht;
}

int hash_table_insert_mag(lotto* lotto, int hashed, char* key){
    head_mag* p=magazzino->elements[hashed], *p_del=NULL;
    head_mag* testa;
    while(p!=NULL && strcmp(key, p->key)){
        p_del=p;
        p=p->next_item;
    }
    if(p==NULL){ //creo la testa e inserisco
        if(p_del==NULL){//primo lotto del bucket
            magazzino->elements[hashed]=malloc(sizeof(head_mag));
            testa=magazzino->elements[hashed];
        }
        else{
            p_del->next_item=malloc(sizeof(head_mag));
            testa=p_del->next_item;
        }
        strcpy(testa->key,key);
        testa->peso_tot=lotto->peso;
        testa->numero=1;
        testa->next_same=lotto;
        lotto->next=NULL;
        testa->next_item=NULL;
    }
    else{
        p->peso_tot+=lotto->peso;
        p->numero++;
        inserisci_in_ordine(p,lotto);
    }
    magazzino->occupied++;
    return hashed;
}

void inserisci_in_ordine(head_mag* testa, lotto* lot){
    lotto* p;
    lotto* p_del=NULL;
    p=testa->next_same;
    while(p!=NULL && p->scadenza<lot->scadenza){
        p_del=p;
        p=p->next;
    }
    lot->next=p;
    if(p_del!=NULL){
        p_del->next=lot;
    }
    else{
        testa->next_same=lot;
    }
    return;
}

int hash_table_insert_ric(char* key, int hashed){
    head_ric* p=ricettario->elements[hashed];
    head_ric* p_del=NULL;
    head_mag* magazza, *magazza_del;
    int quantita;
    char ingr[MAX_LEN];
    char c;
    ingrediente* new;
    while(p!=NULL && strcmp(key, p->key)){
        p_del=p;
        p=p->next_item;
    }
    if(p!=NULL){
        c=getc(stdin);
        while(c!='\n'){
            assert(scanf("%s %d",ingr,&quantita)); //da ottimizzare leggendo tutta la riga in un colpo e buttandola (gets forse?)
            c=getc(stdin);
        }
        return -1; //ricetta già presente
    }
    else{
        //inizializzo la testa
        if(p_del!=NULL){
            p_del->next_item=malloc(sizeof(head_ric));
            p_del=p_del->next_item;
        }
        else{
            ricettario->elements[hashed]=malloc(sizeof(head_ric));
            p_del=ricettario->elements[hashed];
        }
        strcpy(p_del->key,key);
        p_del->peso_tot=0;
        p_del->ordiniAttivi=0;
        p_del->numero=0;
        p_del->next_item=NULL;
        p_del->next_same=NULL;
        c=getc(stdin);
        while(c!='\n'){
            assert(scanf("%s %d", ingr,&quantita));
            new=malloc(sizeof(ingrediente));
            strcpy(new->key,ingr);
            new->peso=quantita;
            new->next=p_del->next_same;
            //ricerca del lotto associato; se assente lo creo mettendo la quantità a zero
            int hashed_ingr=hash(ingr) & MAG_SIZE_HEX;
            magazza=magazzino->elements[hashed_ingr];
            magazza_del=magazza;
            while(magazza!=NULL && strcmp(magazza->key,ingr)){
                magazza_del=magazza;
                magazza = magazza->next_item;
            }
            if(magazza==NULL){
                if(magazza_del!=NULL){
                    magazza_del->next_item=malloc(sizeof(head_mag));
                    magazza=magazza_del->next_item;
                    strcpy(magazza->key, ingr);
                    magazza->peso_tot=0;
                    magazza->numero=0;
                    magazza->next_same=NULL;
                    magazza->next_item=NULL;
                }
                else{
                    magazzino->elements[hashed_ingr]=malloc(sizeof(head_mag));
                    magazza=magazzino->elements[hashed_ingr];
                    strcpy(magazza->key, ingr);
                    magazza->peso_tot=0;
                    magazza->numero=0;
                    magazza->next_same=NULL;
                    magazza->next_item=NULL;
                }

            }
            new->nel_magazzino=magazza;
            p_del->numero++;
            p_del->peso_tot+=quantita;
            p_del->next_same=new;
            c=getc(stdin);
        }
        ricettario->occupied++;
        return hashed;
    }
}

void rimuovi_ricetta(){
    char ric[MAX_LEN];
    head_ric* p_del=NULL;
    assert(scanf("%s",ric));
    int hashed=hash(ric)&MAG_SIZE_HEX;//ricsize
    head_ric* p=ricettario->elements[hashed];
    while(p!=NULL && strcmp(p->key,ric)){
        p_del=p;
        p=p->next_item;
    }
    if(p==NULL){
        printf("non presente\n");
        return;
    }
    else{
        if(p->ordiniAttivi>0){
            printf("ordini in sospeso\n");
            return;
        }
        else{
            if(p_del==NULL){ //se è la prima
                ricettario->elements[hashed]=p->next_item;
                cancella_ricetta(p);
                free(p);
            }
            else{//rimozione in mezzo
                p_del->next_item=p->next_item;
                cancella_ricetta(p);
                free(p);
            }
            printf("rimossa\n");
            ricettario->occupied--;
            return;
        }
    }

}

void cancella_ricetta(head_ric* testa){
    ingrediente* p=testa->next_same, *temp=p;
    while(temp!=NULL){
        temp=p->next;
        free(p);
        p=temp;
    }
    return;
}

void rifornimento(){
    char ingrediente[MAX_LEN], a;
    int peso,scadenza;
    lotto* new_lotto;
    a=getc(stdin);
    while(a!='\n'){
        assert(scanf("%s %d %d",ingrediente, &peso, &scadenza));
        if(scadenza>t){
            new_lotto=crea_lotto(peso,scadenza);
            hash_table_insert_mag(new_lotto,hash(ingrediente)&MAG_SIZE_HEX,ingrediente);
            if(scadenza<scad_min){ //se il t<scadenza<scad_min, ho una nuova scad_min
                scad_min=scadenza;
            }
        }
        a=getc(stdin);
    }
    return;
}

lotto* crea_lotto(int peso, int scadenza){
    lotto* p=malloc(sizeof(lotto));
    p->peso=peso;
    p->scadenza=scadenza;
    p->next=NULL;
    return p;
}

head_mag* lookup_magazzino(char* key){
    int hashed=hash(key)&MAG_SIZE_HEX;
    head_mag* p=magazzino->elements[hashed];
    while(p!=NULL && strcmp(p->key,key)){
        p=p->next_item;
    }
    return p;
}

head_ric* lookup_ricettario(char* key){
    int hashed=hash(key)&MAG_SIZE_HEX;//ricsize
    head_ric* p=ricettario->elements[hashed];
    while(p!=NULL && strcmp(p->key,key)){
        p=p->next_item;
    }
    return p;
}

int gestione_ordine(char* ricetta, int unita){
    head_ric* ric;
    head_mag* ingr;
    ingrediente* p_ric;
    int peso_richiesto,i=0;
    coda elem;
    int* quantita_richieste;

    ric=lookup_ricettario(ricetta);
    if(ric!=NULL){
        quantita_richieste=malloc((sizeof(int))*ric->numero);
        p_ric=ric->next_same;
        while(p_ric!=NULL){
            peso_richiesto=(p_ric->peso)*unita;
            ingr=p_ric->nel_magazzino;
            if(peso_richiesto>ingr->peso_tot){
                //metti in attesa e return
                elem=crea_elem_coda(ric,unita,false);
                inserisci_elem_coda(elem);
                ric->ordiniAttivi++;
                free(quantita_richieste);
                return 0;
            }
            quantita_richieste[i]=peso_richiesto;
            i++;
            p_ric=p_ric->next;
        }
        //prepara effettivamente
        prepara_ordine(ric,unita,quantita_richieste);
        elem=crea_elem_coda(ric,unita,true);
        inserisci_elem_coda(elem);
        ric->ordiniAttivi++;
        free(quantita_richieste);
        return 1;
    }
    else{
        return -1;
    }
}

coda crea_elem_coda(head_ric* p_ric, int unita, bool isReady){
    coda queue=malloc(sizeof(ElemCoda));
    queue->p_ric=p_ric;
    queue->unita=unita;
    queue->peso=(p_ric->peso_tot)*unita;
    queue->data=t;
    queue->next=NULL;
    queue->prev=NULL;
    queue->isReady=isReady;
    return queue;
}

void inserisci_elem_coda(coda elem){
    if(elem->isReady==true){
        if(pronti==NULL){//se è il primo elemento nella coda dei pronti
            pronti=elem;
            fine_pronti=elem;
            elem->prev=NULL;
            elem->next=NULL;
            return;
        }
        else{
            fine_pronti->next=elem;
            elem->prev=fine_pronti;
            elem->next=NULL;
            fine_pronti=elem;
            return;

        }
    }
    //altrimenti devo metterlo in attesa
    if(attese==NULL){//se è il primo elemento nella coda dei pronti
            attese=elem;
            fine_attese=elem;
            elem->prev=NULL;
            elem->next=NULL;
            return;
    }
    else{
        fine_attese->next=elem;
        elem->prev=fine_attese;
        elem->next=NULL;
        fine_attese=elem;
        return;
    }
}

void prepara_ordine(head_ric* ric, int unita, int* quantita_richieste){
    ingrediente* p_ingr=ric->next_same;
    head_mag* p_mag;
    lotto* p_lot, *temp;
    int peso,i=0;
    bool sent=false;

    while(p_ingr!=NULL){
        peso=quantita_richieste[i];
        p_mag=p_ingr->nel_magazzino;
        p_lot=p_mag->next_same;
        while(peso>p_lot->peso){
            peso-=p_lot->peso;
            temp=p_lot;
            p_lot=p_lot->next;
            if(temp->scadenza==scad_min){
                sent=true;
            }
            free(temp);
            p_mag->next_same=p_lot;
            p_mag->numero--;
            magazzino->occupied--;
        }
        p_lot->peso-=peso;
        if(p_lot!=NULL && p_lot->peso==0){
            temp=p_lot;
            p_lot=p_lot->next;
            if(temp->scadenza==scad_min){
                sent=true;
            }
            free(temp);
            p_mag->next_same=p_lot;
            p_mag->numero--;
            magazzino->occupied--;
        }
        p_mag->peso_tot-=quantita_richieste[i];
        i++;
        p_ingr=p_ingr->next;
    }
    num_pronti++;
    if(sent){
        aggiorna_scadmin();
    }
    return;
}

void aggiorna_scadmin(){ //ottimizzazzione; incrementa occupied solo quando aggiungi la TESTA di un lotto, decrementandola così meno volte 
    int i,scad_min_interessante=scad_min,count=magazzino->occupied; // se trovo un lotto che scade in scad_min e non l'ho usato nella preparazione, essa non cambia!
    head_mag* p;
    lotto* p_lot;
    bool first=true,stop=false;
    if(magazzino->occupied==0){
        scad_min=INFTY;
    }

    for(i=0;i<magazzino->size;i++){
        p=magazzino->elements[i];
        while(p!=NULL){
            if(count==0){//se le restanti celle del magazzino sono vuote non sto a controllarle
                return;
            }
            p_lot=p->next_same;
            while(p_lot!=NULL&&!stop){
                count--;
                if(first || p_lot->scadenza<=scad_min){
                    scad_min=p_lot->scadenza;
                    if(p_lot->scadenza==scad_min_interessante){ //ho trovato un lotto che scade nella scad_min attuale! Resta la stessa
                        return;
                    }
                    first=false;
                }
                else{
                    stop=true;
                }
                p_lot=p_lot->next;
            }
            stop=false;
            p=p->next_item;
        }

    }
    return;
}

void verifica_magazzino(){
    head_mag* p;
    lotto* p_lot,*temp;
    int i,count=magazzino->occupied;
    bool first=true;

    for(i=0;i<magazzino->size;i++){
        if(magazzino->occupied==0){ //se il magazzino non ha più elementi termino il controllo
            scad_min=INFTY; //scad_min torna "infinito"
            return;
        }
        p=magazzino->elements[i];
        while(p!=NULL){
            p_lot=p->next_same;
            while(p_lot!=NULL && p_lot->scadenza==t){
                count--;
                temp=p_lot->next;
                p->next_same=temp;
                p->peso_tot-=p_lot->peso;
                p->numero--;
                free(p_lot);//essendo scaduto, lo cancello
                p_lot=temp;
                magazzino->occupied--;
            }
            if(p_lot!=NULL && ((first) || (scad_min>p_lot->scadenza))){
                scad_min=p_lot->scadenza;
                first=false;
            }
            if(count==0){
                return;
            }
            p=p->next_item;
        }
    }
    return;
}

void gestione_attesa(){
    coda queue=attese, da_inserire;
    head_ric* p_ric;
    ingrediente* p_ingr;
    head_mag* p_mag;
    bool preparabile=true;
    int* quantita_richieste;
    int i, peso_richiesto;
    while(queue!=NULL){
        p_ric=queue->p_ric;
        quantita_richieste=malloc(sizeof(int)*p_ric->numero);
        i=0;
        p_ingr=p_ric->next_same;
        while(preparabile && p_ingr!=NULL){
            peso_richiesto=(p_ingr->peso)*queue->unita;
            p_mag=p_ingr->nel_magazzino;
            if(p_mag->peso_tot >= peso_richiesto){
                quantita_richieste[i]=peso_richiesto;
                p_ingr=p_ingr->next;
            }
            else{
                preparabile=false;
            }
            i++;
        }
        if(preparabile){
            prepara_ordine(p_ric,queue->unita,quantita_richieste);
            queue->isReady=true;
            //rimozione dalla coda delle attese
            da_inserire=queue;
            queue=queue->next;
            if(da_inserire->next==NULL){//se sto togliendo l'ultimo elemento
                fine_attese=da_inserire->prev;
            }
            if(da_inserire->prev!=NULL){ //se non sto togliendo il primo elemento
                da_inserire->prev->next=queue;
            }
            else{ //se sto togliendo il primo elemento
                attese=queue;
                if(attese!=NULL){ //se attese non è l'ultimo elemento'
                    attese->prev=NULL;
                }
            }
            if(attese!=NULL && queue!=NULL){
                queue->prev=da_inserire->prev;
            }
            da_inserire->next=NULL;
            da_inserire->prev=NULL;
            inserisci_pronti_cronologico(da_inserire);
        }
        else{
            queue=queue->next;
        }
        preparabile=true;
        free(quantita_richieste);
    }
    return;
}

void inserisci_pronti_cronologico(coda elem){
    coda p,p_del;
    if(pronti==NULL){ //se la coda è vuota, inserisco il primo e unico elemento
        pronti=elem;
        fine_pronti=elem;
        elem->next=NULL;
        elem->prev=NULL;
        return;
    }
    p=pronti;
    p_del=NULL;
    while(p!=NULL && elem->data > p->data){
        p_del=p;
        p=p->next;
    }
    if(p_del==NULL){ //inserimento in testa
        elem->next=pronti;
        elem->prev=NULL;
        pronti->prev=elem;
        pronti=elem;
        return;
    }
    else if(p==NULL){//inserimento in coda
        p_del->next=elem;
        elem->next=NULL;
        elem->prev=p_del;
        fine_pronti=elem;
    }
    else{ //inserimento in mezzo
        elem->next=p;
        elem->prev=p_del;
        p_del->next=elem;
        p->prev=elem;
    }
    return;
}

void carica_ordini(){
    coda p=pronti;
    int peso_curr=0,count=0,i;
    coda* da_caricare=malloc(sizeof(coda)*num_pronti); //alloco un vettore di puntatori a ElemCoda grande tanto quanto il numero di ingredienti pronti (nel caso peggiore li caricherò tutti)
    if(pronti!=NULL){
        while(p!=NULL && (peso_curr+p->peso)<=max_carico){
            peso_curr+=p->peso;
            da_caricare[count]=p;
            p=p->next;
            count++;
        }
        if(count==0){
            printf("camioncino vuoto\n");
            free(da_caricare);
            return;
        }
        pronti=p;
        da_caricare=mergeSort(da_caricare,0,count-1);
        for(i=0;i<count;i++){
            printf("%d %s %d\n",da_caricare[i]->data,da_caricare[i]->p_ric->key,da_caricare[i]->unita);
            num_pronti--;
            da_caricare[i]->p_ric->ordiniAttivi--;
            free(da_caricare[i]);
            fflush(stdin);
        }
        free(da_caricare);
        return;
    }
    else{
        printf("camioncino vuoto\n");
        free(da_caricare);
        return;
    }
}

coda* mergeSort(coda* arr, int p, int r){
    //implemento il mergesort che ordina rispetto al peso stabile nella data
    int q;
    if(p<r){
        q=p+(r-p)/2;
        mergeSort(arr,p,q);
        mergeSort(arr,q+1,r);
        arr=merge(arr,p,q,r);
    }
    return arr;
}

coda* merge(coda* arr, int p, int q, int r){
    int len1=q-p+1, len2=r-q,i,j,k;
    coda* L,*R;
    L=malloc(sizeof(coda)*(len1));
    R=malloc(sizeof(coda)*(len2));
    for(i=0;i<len1;i++){
        L[i]=arr[p+i];
    }
    for(i=0;i<len2;i++){
        R[i]=arr[q+i+1];
    }

    i=0;j=0;k=p;
    while(i<len1 && j<len2){
        if(L[i]->peso>=R[j]->peso){
            arr[k]=L[i];
            i++;
        }
        else{
            arr[k]=R[j];
            j++;
        }
        k++;
    }
    while(i<len1){
        arr[k]=L[i];
        i++;
        k++;
    }

    while(j<len2){
        arr[k]=R[j];
        j++;
        k++;
    }
    free(L);
    free(R);
    return arr;
}
