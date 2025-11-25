import matplotlib.pyplot as plt
import pandas as pd

# --- Dados da tabela ---
set = "Chu_Stuckey"

if set == "Challenge":
    data = {
        "subset": [
            "NWRS1","NWRS2","Shaw","NWRS3","NWRS4","SP1","NWRS5","NWRS6","Miller","GP1",
            "GP2","GP3","GP4","SP2","NWRS7","NWRS8","SP3","GP5","GP6","GP7","GP8","SP4"
        ],
        "BKS": [
            3,4,13.68,7,7,9,12,12,13,45,
            40,40,30,19,10,16,34,95,75,75,60,53
        ],
        "PieceRank": [
            3,4,13.92,7,7,9,12,12,13,45,
            40,41,31,22,10,16,40,96,75,75,61,57
        ],
        "S_star": [
            3,4,13.68,7,7,9,12,12,13,45,
            40,40,30,19,10,16,34,95,76,77,62,53
        ]
    }
elif set == "SCOOP":
    data = {
        "subset": [
            "B_CARLET_137","B_22X18_50","B_39Q18_82","B_18AB1_32","B_42F22_93",
            "B_18CR1_33","A_AP-9.d_10","A_AP-9.d_3","B_12F18_11","B_GTM18A_139",
            "A_AP-9.d_11","B_23B25_52","B_12M18_12","A_AP-9.d_6","B_CUC28A_138",
            "B_REVAL_145","A_FA+AA-_15","A_FA+AA-_2","A_FA+AA-_12","A_FA+AA-_6",
            "A_FA+AA-_8","A_FA+AA-_11","A_FA+AA-_1","A_FA+AA-_13"
        ],
        "BKS": [
            5,10,5,6,5,
            4,6,6,6,5,
            6,5,6,5,6,
            7,9,11,9,13,
            11,11,12,17
        ],
        "PieceRank": [
            5,10,5,6,5,
            4,7,6,7,5,
            6,5,7,5,6,
            8,10,13,10,13,
            12,12,14,21
        ],
        "S_star": [
            5,10,5,6,5,
            4,6,6,6,5,
            6,5,6,5,6,
            7,9,11,9,13,
            11,11,12,17
        ]
    }
elif set == "Faggioli_Bentivoglio":
    data = {
        "subset": [
            "p1010","p1020","p1030","p1040","p1050",
            "p1510","p1520","p1530","p1540","p1550",
            "p2010","p2020","p2030","p2040","p2050",
            "p2510","p2520","p2530","p2540","p2550",
            "p3010","p3020","p3030","p3040","p3050",
            "p4010","p4020","p4030","p4040","p4050"
        ],
        "BKS": [
            5.50,6.20,6.10,7.70,8.20,
            6.60,7.20,7.30,7.20,7.40,
            7.50,8.50,8.80,8.50,7.90,
            8.00,9.80,10.50,10.40,10.00,
            7.80,11.10,12.20,12.10,11.20,
            8.40,13.00,14.50,14.90,14.60
        ],
        "PieceRank": [
            5.80,6.80,6.90,8.30,8.60,
            6.60,8.20,9.00,8.60,8.30,
            7.70,8.80,10.10,9.90,9.70,
            8.20,10.10,12.30,11.30,11.70,
            8.10,11.20,13.20,13.00,12.80,
            8.40,13.40,15.60,16.40,16.40
        ],
        "S_star": [
            5.50,6.20,6.10,7.70,8.20,
            6.60,7.20,7.30,7.20,7.40,
            7.50,8.50,8.80,8.50,7.90,
            8.00,9.80,10.50,10.40,10.00,
            7.80,11.10,12.20,12.10,11.20,
            8.40,13.00,14.50,14.90,14.70 
        ]
    }
elif set == "Chu_Stuckey":
    data = {
        "subset": [
            "Random-30-30-2", "Random-30-30-4", "Random-30-30-6", "Random-30-30-8", "Random-30-30-10",
            "Random-40-40-2", "Random-40-40-4", "Random-40-40-6", "Random-40-40-8", "Random-40-40-10",
            "Random-50-50-2", "Random-50-50-4", "Random-50-50-6", "Random-50-50-8", "Random-50-50-10",
            "Random-50-100-2", "Random-50-100-4", "Random-50-100-6", "Random-50-100-8", "Random-50-100-10",
            "Random-75-75-2", "Random-75-75-4", "Random-75-75-6", "Random-75-75-8", "Random-75-75-10",
            "Random-100-50-2", "Random-100-50-4", "Random-100-50-6", "Random-100-50-8", "Random-100-50-10",
            "Random-100-100-2", "Random-100-100-4", "Random-100-100-6", "Random-100-100-8", "Random-100-100-10",
            "Random-125-125-2", "Random-125-125-4", "Random-125-125-6", "Random-125-125-8", "Random-125-125-10"
        ],

        "PieceRank": [
            9.00, 17.00, 22.20, 25.40, 27.00,
            13.00, 21.40, 29.00, 33.60, 35.80,
            15.20, 26.00, 35.80, 41.80, 44.80,
            22.60, 36.20, 43.40, 46.40, 47.80,
            19.20, 37.60, 52.40, 60.40, 65.80,
            15.40, 32.20, 48.60, 61.00, 70.80,
            24.00, 48.80, 67.40, 78.60, 86.00,
            27.00, 58.20, 82.20, 96.80, 105.40
        ],

        "BKS": [
            8.60, 16.00, 21.20, 25.20, 27.00,
            11.00, 20.20, 28.20, 32.60, 35.40,
            13.20, 23.80, 35.20, 40.40, 44.40,
            21.20, 34.60, 42.40, 46.20, 47.80,
            15.80, 34.20, 50.00, 59.40, 64.80,
            11.60, 26.40, 44.00, 56.20, 67.20,
            20.20, 45.00, 64.60, 76.60, 84.60,
            22.80, 53.00, 78.40, 93.60, 103.20
        ],

        "S_star": [
            8.60, 16.00, 21.20, 25.20, 27.00,
            11.00, 20.20, 28.20, 32.60, 35.40,
            13.20, 23.80, 35.20, 40.40, 44.40,
            21.20, 34.60, 42.40, 46.20, 47.80,
            16.00, 34.20, 50.00, 59.40, 64.80,
            11.60, 26.40, 44.00, 56.20, 67.20,
            20.20, 45.40, 64.60, 76.60, 84.60,
            23.60, 53.40, 78.80, 93.80, 103.20
        ]
    }
elif set == "Carvalho_Soma":
    data = {
        "subset": [
            "Random-150-150-2", "Random-150-150-4", "Random-150-150-6", "Random-150-150-8", "Random-150-150-10",
            "Random-175-175-2", "Random-175-175-4", "Random-175-175-6", "Random-175-175-8", "Random-175-175-10",
            "Random-200-200-2", "Random-200-200-4", "Random-200-200-6", "Random-200-200-8", "Random-200-200-10"
        ],

        "BKS": [
            25.90, 61.60, 93.20, 111.70, 123.90,
            30.30, 73.70, 107.70, 128.30, 143.50,
            36.00, 84.30, 121.50, 147.10, 162.80
        ],

        "PieceRank": [
            31.90, 68.80, 98.60, 116.10, 126.60,
            37.80, 82.30, 114.70, 133.60, 147.60,
            44.70, 91.80, 129.00, 153.20, 167.60
        ],

        "S_star": [
            26.70, 61.80, 93.70, 111.90, 123.90,
            31.20, 74.30, 108.20, 128.40, 143.60,
            37.00, 83.50, 121.20, 147.20, 163.10
        ]
    }
else:
    raise ValueError("Conjunto de dados inválido. Escolha entre 'Challenge', 'SCOOP', 'Faggioli_Bentivoglio', 'Chu_Stuckey' ou 'Carvalho_Soma'.")

# --- Cores configuráveis ---
color_bks = "green"
color_piece = "red"
color_sstar = "blue"

df = pd.DataFrame(data)

plt.style.use("seaborn-v0_8-whitegrid")
plt.figure(figsize=(16,8), dpi=200)

plt.plot(df["subset"], df["BKS"], label="OPT", color=color_bks)
plt.plot(df["subset"], df["PieceRank"], label="PieceRank", color=color_piece)
plt.plot(df["subset"], df["S_star"], label="PT(S*)", linestyle="--", color=color_sstar)

plt.yscale("log")
plt.xlabel("Subset", fontsize=14)
plt.ylabel("Solution Value (Logarithmic Scale)", fontsize=14)
plt.title("Comparison of OPT, PieceRank, and S* by subset (log scale)", fontsize=16)
plt.xticks(rotation=90, fontsize=12)
plt.legend(loc="upper left", fontsize=12)
plt.tight_layout()

output_filename = "graph_"+ set +".png"
plt.savefig(output_filename, format="png", dpi=300, bbox_inches="tight")