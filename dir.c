/*
	* Disciplina: Sistemas Operacionais
	* Professor: Roberto Souza
	* Equipe: Alfonso Martinez, João Pedro Barbosa e Daniel Tavares
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef struct descritor {

    char *nome_do_arquivo;

    char extensao;

    int atributos;

    char reservado[10];

    char hora[2];

    char data[2];

    char primeiro_cluster[2];

    int tamanho;

} DESCRITOR;


void remove_espacos(char* k)
{
	char* i = k;
	char* j = k;
	while(*j != 0)
	{
		*i = *j++;
		if (*i != ' ')
		{
			i++;
		}
	}
	*i = 0;
}

//Pegar bytes por setor
int bytes_por_setor(char* a)
{
	return (int) (a[11] | a[12] << 8);
}
//Pegar atributos do arquivo
int atributos_dr(char* a, int endereco)
{
	int end_atributo = endereco + 11;
	int atributo = a[end_atributo] & 0xFF;

	if ((atributo & 0x0F) == 0x0F || (atributo & 0x08) == 0x08)
	{
		return -1;
	}
	else if (atributo & 0x10 == 0x10)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

//Pegar o tamanho do arquivo
int tamanho_arq(char* a, int i)
{
	return (int)((a[i + 28] & 0xFF) 
		| ((a[i + 29] & 0xFF) << 8) 
		| ((a[i + 30] & 0xFF) << 16) 
		| ((a[i + 31] & 0xFF) << 24));
}

//Pegar o nome do arquivo
char* nome_arq(char* a, int i)
{
	char* nomeArq = malloc(sizeof(char));
	if (nomeArq == 0)
	{
		printf("Falha ao alocar espaco.\n");
		exit(1);
	}

	for (int j = 0; j < 8; j++)
	{
		nomeArq[j] = a[i + j];
	}

	nomeArq[8] = '.';

	for (int j = 9; j < 12; j++)
	{
		nomeArq[j] = a[i + j - 1];
	}

	remove_espacos(nomeArq);

	return nomeArq;
}





int main(int argc, char* argv[])
{
	DESCRITOR ds;

    if (argc != 2)
    {
        printf("E necessario passar o arquivo imagem como argumento\n");
        exit(1);
    }

    /* Abrir a imagem do disco */
    int fd;
    if ((fd = open(argv[1], O_RDONLY)) == -1) 
    {
    	printf("Falha ao abrir a imagem do disco.\n");
    	exit(1);
    }

    /* Mapear a imagem do disco para um vetor */
    struct stat dv;
	if (fstat(fd, &dv) == -1) exit(1);

	char* p = mmap(NULL, dv.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED)
    {
        printf("Falha ao mapear a imagem do disco.\n");
        exit(1);
    }

	int bytes_setor = bytes_por_setor(p);
	int end_raiz = 19 * bytes_setor;
	int tam_raiz = 14 * bytes_setor;

	/* Loop para printar todos os arquivos do diretório raiz. */
	for (int i = end_raiz; i < end_raiz + tam_raiz; i += 32)
	{
		ds.atributos = atributos_dr(p, i); 
		if (ds.atributos != -1 && (int)p[i] != 0)
		{
			/* Pegar o tipo, tamanho do arquivo, e o seu nome*/
			ds.extensao = ds.atributos == 0 ? 'D' : 'F';
			ds.tamanho = tamanho_arq(p, i);
			ds.nome_do_arquivo = nome_arq(p, i);

			/* Pegar a hora de criação do arquivo */
			int hora_criacao = (int)((p[i + 14] & 0xFF) | (p[i + 15] & 0xFF) << 8);
			int hora = hora_criacao >> 11;
			int minuto = (hora_criacao >> 5) & 0x3F;
			int segundo =  (hora_criacao & 0x1F) * 2;

			/* Pegar a data de criação do arquivo */
			int data_criacao = (int)((p[i + 16] & 0xFF) | (p[i + 17] & 0xFF) << 8);
			int ano = 1980 + (data_criacao >> 9);
			int mes = (data_criacao >> 5) & 0x1F;
			int dia = data_criacao & 0x1F;

			/* Printar os atributos do arquivo do diretório raiz */
			printf("Tipo de arquivo: %c\n", ds.extensao);
			printf("Tamanho do arquivo: %d\n", ds.tamanho);
			printf("Nome do arquivo: %s \n", ds.nome_do_arquivo);
			printf("Data/Hora: %d/%02d/%02d %02d:%02d:%02d\n", ano, mes, dia, hora, minuto, segundo);
			printf("\n================================\n");
		}
	}

	/* Desmapear a memoria de fechar os arquivos. */
	if (munmap(p, dv.st_size) == -1)
    {
        printf("Falha ao tentar desmapear.\n");
        exit(1);
    }

	if (close(fd) == -1)
    {
        printf("Falha ao tentar fechar o arquivo.\n");
        exit(1);
    }

	return 0;
}
