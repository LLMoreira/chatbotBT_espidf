#ifndef SAVEFILE_H
#define SAVEFILE_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

void init_spiffs(void);                 //Inicia a partição
void createFileConfig(void);            //Cria o arquivo de configuração
const char* readFileConfig(void);              //Lê a informações do arquivo de configuração
void writeFileConfig(const char *data); //Escreve no arquivo de configuração
void deleteFileConfig(void);            //Deleta o arquivo de configuração

#ifdef __cplusplus
}
#endif

#endif // SAVEFILE_H
