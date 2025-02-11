from config import RAW_ROOT_PATH, HPC_ROOT_path, PARSER_PATH, PARSER_PROFILE_PATH
import os
import shutil


def main():
    for label in ['ransomware', 'benign']:
        for file in os.listdir(f'{RAW_ROOT_PATH}\\{label}'):
            if not file.endswith('etl'):
                continue

            sample = file.split('.')[0]
            
            print(f'start process {file}')
            try:
                # parse ETL file
                os.system(f'wpaexporter.exe /tti -i {RAW_ROOT_PATH}\\{label}\\{file} -profile {PARSER_PROFILE_PATH} -outputfolder {HPC_ROOT_path}\\{label}\\{sample}')
                os.rename(f'{HPC_ROOT_path}\\{label}\\{sample}\\PMC_Summary_Table_test.csv', f'{HPC_ROOT_path}\\{label}\\{sample}.csv')
                shutil.rmtree(f'{HPC_ROOT_path}\\{label}\\{sample}')

                # get ETL start time
                os.system(f'{PARSER_PATH} {RAW_ROOT_PATH}\\{label}\\{sample}.etl {HPC_ROOT_path}\\{label}\\{sample}_starttime.txt')
            except:
                print(f'error process {RAW_ROOT_PATH}\\{label}\\{file}')
                continue
            print(f'finish process {RAW_ROOT_PATH}\\{label}\\{file}')


if __name__ == '__main__':
    main()
