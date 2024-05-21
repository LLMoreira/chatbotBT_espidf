#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

static const char *TAG = "savefile";

void init_spiffs(void)
{   //Inicia a partição
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/storage",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

#ifdef CONFIG_EXAMPLE_SPIFFS_CHECK_ON_START
    ESP_LOGI(TAG, "Performing SPIFFS_check().");
    ret = esp_spiffs_check(conf.partition_label);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
        return;
    } else {
        ESP_LOGI(TAG, "SPIFFS_check() successful");
    }
#endif

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);
        return;
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    // Verificar a consistência da informação do tamanho da partição reportada.
    if (used > total) {
        ESP_LOGW(TAG, "Number of used bytes cannot be larger than total. Performing SPIFFS_check().");
        ret = esp_spiffs_check(conf.partition_label);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
            return;
        } else {
            ESP_LOGI(TAG, "SPIFFS_check() successful");
        }
    }    

    /*// Verifica se o ficheiro de destino existe antes de o renomear
    struct stat st;
    if (stat("/storage/foo.txt", &st) == 0) {
        // Apagar se existir
        unlink("/storage/foo.txt");
    }

    // Mudar o nome do ficheiro original
    ESP_LOGI(TAG, "Renaming file");
    if (rename("/storage/hello.txt", "/storage/foo.txt") != 0) {
        ESP_LOGE(TAG, "Rename failed");
        return;
    }

   

    // Tudo feito, desmonte a partição e desactive o SPIFFS
    esp_vfs_spiffs_unregister(conf.partition_label);
    ESP_LOGI(TAG, "SPIFFS unmounted");*/
}

void createFileConfig(void){
    //Cria o arquivo de configuração
    ESP_LOGI(TAG, "Criando arquivo de configuração");
    FILE* f = fopen("/storage/config_file.txt", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }
    fprintf(f, "Sem configuração\n");
    fclose(f);
    ESP_LOGI(TAG, "File written");
}

const char* readFileConfig(void) {
    static char response[128]; // Buffer estático para armazenar a resposta
    ESP_LOGI(TAG, "Lendo configuração do dispositivo");
    FILE *f = fopen("/storage/config_file.txt", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha na leitura do arquivo");
        snprintf(response, sizeof(response), "Ainda não existe configuração\n");
        return response;
    }
    char line[64];
    fgets(line, sizeof(line), f);
    fclose(f);
    char* pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Configuração atual: '%s'", line);
    snprintf(response, sizeof(response), "Configuração atual: '%s'\n", line);
    return response;
}

void writeFileConfig(const char *data){
    //Escreve no arquivo de configuração
    ESP_LOGI(TAG, "Escrevendo no arquivo de configuração");
    FILE* f = fopen("/storage/config_file.txt", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha na escrita");
        return;
    }
    fprintf(f, "%s\n", data);
    fclose(f);
    ESP_LOGI(TAG, "Essas são as configurações atualizadas: '%s'", data);
}

void deleteFileConfig(void){
    //Deleta o arquivo de configuração
    unlink("/storage/config_file.txt");
    ESP_LOGI(TAG, "Arquivo apagado");
}