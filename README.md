# RansomRadar

This repository contains the code and dataset for the paper *Ransomware Detection Through Temporal Correlation between Encryption and I/O Behavior*. 

The folder tree for this repository is shown below:

```
├── code
├── dataset
├── models
├── monitor
├── result
├── helper
└── features
    ├── 1s
    ├── 100ms
    └── lstm
```

`code` stores the python analysis code. `dataset` stores all the raw HPC and IRP data. `features` stores all the features extracted from raw data. `models` stores the KNN model for encryption detection and LSTM model for temporal correlation detection. `monitor` stores the tools for monitoring system data. `helper` stores the C# code for parsing windows ETL file. `result` stores the detection result.

(Due to the limit of of GitHub repository, we uploaded parts of the dataset as demo. We will make the whole dataset public on google drive after paper publication.)

## Environment Setup

**Requirement**:

- OS: Windows 10/11, with Windows ADK installed

- Python: >= 3.7 

**Python package**

```cmd
joblib==1.3.2
numpy==1.26.4
pandas==2.2.1
scikit_learn==1.3.1
torch==2.2.1
```

## Usage

0. system monitor

   1. start Hardware Monitor

      we use wpr on windows to monitor hardware data

      ```cmd
      wpr -start /path/to/record.wprp -filemode
      ```

   2. start IO Monitor

      we use minifilter driver to monitor irp data, after compile the minifilter driver and install it in the os

      ```cmd
      sc start irpcollection
      ```

1. configure project path in code/config.py

​	Set the absolute path of the project to the variable PROJECT_PATH.

2. preprocess the windows ETL file

​	`python step0_preprocess.py`

​	This will transform the windows ETL file into csv format for use.

3. extract all the features

​	`python step1_extract_feature.py`

​	This will extract the features for encryption detection and temporal correlation detection. The calculated features will be stored in `features/1s` and `features/lstm` respectively.

4. do encryption detection

​	`python step2_encryption_detection.py`

​	This will do the encryption detection on every second and generate the result in `result/encryption_detection_result_benign` and `resultencryption_detection_result_ransomware`.

5. do temporal correlation detection

​	`python step3_temporal_correlation_detection.py`

​	This will do the temporal correlation detection on every second and generate the result in `result/tc_detection_result_benign` and `result/tc_detection_result_ransomware`.

6. make final judgement

​	`python step4_final_result.py`

​	This is will combine the result from the above two steps and render the final result.



Note that since the original HPC and IRP files are large, we have conducted the step1 and step2 and store the results in `results`. You can start from step3.