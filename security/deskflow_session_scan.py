    # Authentication tests
    b"\x00\x01\x00\x08\x00",      # Empty client name
    b"\x00\x01\x00\x08guest\x00", # Guest client name
    b"\x00\x01\x00\x08synergy\x00", # Original project name
    b"\x00\x01\x00\x08deskflow\x00", # Current project name
    b"\x00\x01\x00\x08Synergy\x00", # Case variations
    b"\x00\x01\x00\x08Deskflow\x00",
    b"\x00\x01\x00\x08SYNERGY\x00",
    b"\x00\x01\x00\x08DESKFLOW\x00",
    b"\x00\x01\x00\x08" + b"A" * 100 + b"\x00", # Long client name