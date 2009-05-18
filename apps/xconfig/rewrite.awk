/^CONFIG_ARCH_[^_]*=y/ {
    ARCH=$3;
}

/^CONFIG_ARCH_[^_]*_[^_]*=y/ {
    if ($3 == ARCH) {
	PLAT=$4;
	printf(",s/^PLATFORM=.*/PLATFORM=%s/\n", tolower(PLAT))
    }
}

/^LBS0_[^_]*=/ {
    if ($2 == ARCH) {
	printf(",s/^SIGMA0_LINKBASE=.*/SIGMA0_LINKBASE=%s/\n", $3)
    }
}

/^LBS0_[^_]*_[^_]*=/ {
    if ($2 == ARCH && $3 == PLAT) {
	printf(",s/^SIGMA0_LINKBASE=.*/SIGMA0_LINKBASE=%s/\n", $4)
    }
}

/^LBRT_[^_]*=/ {
    if ($2 == ARCH) {
	printf(",s/^ROOTTASK_LINKBASE=.*/ROOTTASK_LINKBASE=%s/\n", $3)
    }
}

/^LBRT_[^_]*_[^_]*=/ {
    if ($2 == ARCH && $3 == PLAT) {
	printf(",s/^ROOTTASK_LINKBASE=.*/ROOTTASK_LINKBASE=%s/\n", $4)
    }
}

END {
    printf "wq\n"
}

