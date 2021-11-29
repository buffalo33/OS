import numpy as np
import subprocess

# All tests (to complete if needed)
array = ['11', '12', '21', '22', '23', '31', '32']

# Executes all tests
for i in range(len(array)):
    if i == 51:
        subprocess.run(["python3", "-u", "graph.py", str(array[i])], "--no-seq", "--pdf", stdout=subprocess.DEVNULL)
    else:
        subprocess.run(["python3", "-u", "graph.py", str(array[i]), "--no-seq", "--pdf", "-n", "1000", "-s", "50", "-i", "10"], stdout=subprocess.DEVNULL)

# creates an array to unite pdf
array_pdf_names = ["pdfunite"]

for fileNumber in range(len(array)):
    array_pdf_names.append(f"graph_{array[fileNumber]}.pdf")

array_pdf_names.append("graphs.pdf")

# Execute the unification
subprocess.run(array_pdf_names, stdout=subprocess.DEVNULL)

array_pdf_names.remove("pdfunite")
array_pdf_names.remove("graphs.pdf")

array_pdf_names.insert(0, "rm")

# Remove all pdf générated pdf except graphs.pdf
subprocess.run(array_pdf_names, stdout=subprocess.DEVNULL)
