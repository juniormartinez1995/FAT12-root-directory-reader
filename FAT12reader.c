#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

//#include "diskutils.h"


//removeSpaces
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

//getBytesPerSector
int bytes_por_setor(char* a)
{
	return (int) (a[11] | a[12] << 8);
}
//getFileAttribute
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
//getFileSize
int tamanho_arq(char* a, int i)
{
	return (int)((a[i + 28] & 0xFF) 
		| ((a[i + 29] & 0xFF) << 8) 
		| ((a[i + 30] & 0xFF) << 16) 
		| ((a[i + 31] & 0xFF) << 24));
}
//getFileName
char* nome_arq(char* a, int i)
{
	char* nomeArq = malloc(sizeof(char));
	if (nomeArq == 0)
	{
		printf("Malloc failed.\n");
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

	int bytesPerSector = bytes_por_setor(p);
	int rootAddress = 19 * bytesPerSector;
	int rootSize = 14 * bytesPerSector;

	/* Loop para printar todos os arquivos do diretório raiz. */
	for (int i = rootAddress; i < rootAddress + rootSize; i += 32)
	{
		int attribute = atributos_dr(p, i);
		if (attribute != -1 && (int)p[i] != 0)
		{
			/* Pegar o tipo, tamanho do arquivo, e o seu nome*/
			char type = attribute == 0 ? 'D' : 'F';
			int fileSize = tamanho_arq(p, i);
			char* fileName = nome_arq(p, i);

			/* Pegar a hora de criação do arquivo */
			int creationTime = (int)((p[i + 14] & 0xFF) | (p[i + 15] & 0xFF) << 8);
			int hour = creationTime >> 11;
			int minute = (creationTime >> 5) & 0x3F;
			int second =  (creationTime & 0x1F) * 2;

			/* Pegar a data de criação do arquivo */
			int creationDate = (int)((p[i + 16] & 0xFF) | (p[i + 17] & 0xFF) << 8);
			int year = 1980 + (creationDate >> 9);
			int month = (creationDate >> 5) & 0x1F;
			int day = creationDate & 0x1F;

			/* Printar os atributos do arquivo do diretório raiz */
			printf("Tipo de arquivo: %c\n ", type);
			printf("Tamanho do arquivo: %d\n ", fileSize);
			printf("Nome do arquivo: %s \n", fileName);
			printf("Data/Hora: %d/%02d/%02d %02d:%02d:%02d\n", year, month, day, hour, minute, second);
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
