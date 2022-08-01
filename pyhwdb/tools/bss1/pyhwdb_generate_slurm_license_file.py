#!/usr/bin/env python
"""
Generate slurm license file from hwdb file.
"""
import argparse
import datetime
import pyhwdb
import pyhalco_hicann_v2 as C


def generate_license_strings(db):
    """
    Generates comma separated list containing all SLURM licenses from an HWDB.
    :param db: hwdb instance from which the license string is generated
    :return: tuple of license and TRES strings
    """

    slurm_licenses = []

    wafers = db.get_wafer_coordinates()

    for wafer_coord in wafers:
        wafer = db.get_wafer_entry(wafer_coord)

        for fpga in wafer.fpgas.keys():
            slurm_licenses.append(C.slurm_license(fpga))
            slurm_licenses.append(
                C.slurm_license(
                    C.TriggerGlobal(
                        fpga.toFPGAOnWafer().toTriggerOnWafer(), wafer_coord
                    )
                )
            )
        # ananas licenses are only used for firewall rules, restriction to
        # individual slices is done via trigger group licenses. We therefore
        # add as many ananas licenses as there are slices of an ananas board
        for ananas in wafer.ananas.keys():
            slurm_licenses.append(
                C.slurm_license(ananas) +
                ":" + str(C.AnanasSliceOnAnanas.size))
        for adc in wafer.adcs:
            slurm_licenses.append(str(adc.value.coord))
        # append aggregator licenses for HX multi chip setups which start
        # at Wafer Id 80
        if wafer_coord.value() >= 80:
            slurm_licenses.append("W{}M0".format(wafer_coord.value()))

    # remove non unique trigger and ADC entries while retaining order
    slurm_licenses = dict.fromkeys(slurm_licenses)

    slurm_licenses_tres = \
        ["License/{0}".format(element) for element in slurm_licenses]

    license_out = "Licenses={}".format(",".join(slurm_licenses))
    license_tres_out = "AccountingStorageTRES={}".format(
        ",".join(slurm_licenses_tres)
    )
    return license_out, license_tres_out


def create_license_files(db, license_file_name, tres_file_name):
    """
    Creates slurm license and TRES files from an HWDB instance.
    :param db: hwdb instance from which files are created
    :param license_file_name: path name for license file
    :param tres_file_name: path name for TRES file
    """

    license_out, license_tres_out = generate_license_strings(db)

    # Format header of license file
    time_stamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    license_header = "# file generated on: {}\n\n".format(time_stamp)
    with open(license_file_name, "w") as file:
        file.write(license_header+license_out)
    with open(tres_file_name, "w") as file:
        file.write(license_header+license_tres_out)


if __name__ == "__main__":

    default_path = pyhwdb.database.get_default_path()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--hwdb", type=str, default=default_path, help="path to hwdb yaml file"
    )
    parser.add_argument(
        "--license_file",
        type=str,
        default="licenses",
        help="full path and name of licenses output file",
    )
    parser.add_argument(
        "--tres_file",
        type=str,
        default="accountingStorageTRES",
        help="full path and name of tres output file",
    )

    args = parser.parse_args()

    db = pyhwdb.database()
    db.load(args.hwdb)

    create_license_files(db, args.license_file, args.tres_file)
