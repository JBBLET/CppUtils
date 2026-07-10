import pandas as pd
import numpy as np
import random
import string


def generate_test_data(num_rows=100000, num_fields=50):
    print(f"Generating {num_rows} rows with {num_fields} fields...")

    # 1. Generate IDs (Primary Keys)
    ids = [f"ID_{i:06d}" for i in range(num_rows)]

    # 2. Generate Random Data for 50 columns
    data = {}
    data["UNIQUE_ID"] = ids
    for i in range(1, num_fields):
        col_name = f"FIELD_{i}"
        # Mixture of strings, numbers, and long text
        if i % 5 == 0:  # Long string every 5th column
            data[col_name] = [
                "".join(random.choices(string.ascii_letters, k=50))
                for _ in range(num_rows)
            ]
        else:
            data[col_name] = np.random.randint(0, 1000000, size=num_rows)

    df_ref = pd.DataFrame(data)

    # Create the 'Actual' version by copying Reference
    df_act = df_ref.copy()

    print("Applying modifications...")
    modifications = []

    # --- SCENARIO 1: Value Changes (1% of rows) ---
    change_indices = random.sample(range(num_rows), int(num_rows * 0.01))
    for idx in change_indices:
        col_to_change = f"FIELD_{random.randint(1, num_fields - 1)}"
        old_val = df_act.at[idx, col_to_change]
        new_val = "CHANGED_VAL"
        df_act.at[idx, col_to_change] = new_val
        modifications.append(
            f"MODIFIED: Row {df_act.at[idx, 'UNIQUE_ID']}, Column {col_to_change}"
        )

    # --- SCENARIO 2: Entry Deletions (Remove 500 rows from Actual) ---
    del_indices = random.sample(range(num_rows), 500)
    # We keep them in Ref, but drop from Act
    df_act = df_act.drop(del_indices)
    for i in del_indices:
        modifications.append(f"DELETED: Row {df_ref.at[i, 'UNIQUE_ID']}")

    # --- SCENARIO 3: Entry Additions (Add 500 new rows to Actual) ---
    new_rows_data = []
    for i in range(500):
        new_id = f"NEW_ID_{i}"
        row = {f"FIELD_{j}": "NEW_DATA" for j in range(1, num_fields)}
        row["UNIQUE_ID"] = new_id
        new_rows_data.append(row)
    df_act = pd.concat([df_act, pd.DataFrame(new_rows_data)], ignore_index=True)
    modifications.append("ADDED: 500 new rows")

    # --- SCENARIO 4: Field (Schema) Changes ---
    # Delete a field from Actual
    df_act = df_act.drop(columns=["FIELD_10"])
    modifications.append("FIELD_DELETION: FIELD_10 removed from Actual")

    # Add a new field to Actual
    df_act["NEW_FIELD_X"] = "SURPRISE"
    modifications.append("FIELD_ADDITION: NEW_FIELD_X added to Actual")

    # 3. Save to CSV
    df_ref.to_csv("tests/reference.csv", index=False)
    df_act.to_csv("tests/actual.csv", index=False)

    with open("tests/expected_diffs.txt", "w") as f:
        f.write("\n".join(modifications))

    print("Done! Created 'reference.csv', 'actual.csv', and 'expected_diffs.txt'")


if __name__ == "__main__":
    generate_test_data()
