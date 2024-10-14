# Configuration Examples

## Example 1

```
# sample deskflow configuration file
#
# comments begin with the # character and continue to the end of
# line.  comments may appear anywhere the syntax permits.

section: screens
	# three hosts named:  moe, larry, and curly
	moe:
	larry:
	curly:
end

section: links
	# larry is to the right of moe and curly is above moe
	moe:
		right = larry
		up    = curly

	# moe is to the left of larry and curly is above larry.
	# note that curly is above both moe and larry and moe
	# and larry have a symmetric connection (they're in
	# opposite directions of each other).
	larry:
		left  = moe
		up    = curly

	# larry is below curly.  if you move up from moe and then
	# down, you'll end up on larry.
	curly:
		down  = larry
end

section: aliases
	# curly is also known as shemp
	curly:
		shemp
end
```

## Example 2

```
# sample deskflow configuration file
#
# comments begin with the # character and continue to the end of
# line.  comments may appear anywhere the syntax permits.
# +-------+  +--------+ +---------+
# |Laptop |  |Desktop1| |iMac     |
# |       |  |        | |         |
# +-------+  +--------+ +---------+

section: screens
	# three hosts named:  Laptop, Desktop1, and iMac
	# These are the nice names of the hosts to make it easy to write the config file
	# The aliases section below contain the "actual" names of the hosts (their hostnames)
	Laptop:
	Desktop1:
	iMac:
end

section: links
	# iMac is to the right of Desktop1
	# Laptop is to the left of Desktop1
	Desktop1:
		right = iMac
		left  = Laptop

	# Desktop1 is to the right of Laptop
	Laptop:
		right = Desktop1

	# Desktop1 is to the left of iMac
	iMac:
		left  = Desktop1
end

section: aliases
	# The "real" name of iMac is John-Smiths-iMac-3.local. If we wanted we could remove this alias and instead use John-Smiths-iMac-3.local everywhere iMac is above. Hopefully it should be easy to see why using an alias is nicer
	iMac:
		John-Smiths-iMac-3.local
end

```

## More Complex Example

```
# sample deskflow configuration file
#
# comments begin with the # character and continue to the end of
# line.  comments may appear anywhere the syntax permits.

# This example uses 3 computers. A laptop and two desktops (one a mac)
# They are arranged in the following configuration with Desktop1 acting as the server
# Desktop 2 has 3 screens arranged around desktop1
#
#            +--------+ +---------+
#            |Desktop2| |Desktop2 |
#            |        | |         |
#            +--------+ +---------+
# +-------+  +--------+ +---------+
# |Laptop |  |Desktop1| |Desktop2 |
# |       |  |        | |         |
# +-------+  +--------+ +---------+
#
# The laptop comes and goes but that doesn't really affect this configuration

# The screens section is for the logical or short name of the computers
section: screens
	# three computers that are logically named:  desktop1, desktop2, and laptop
	desktop1:
	desktop2:
	laptop:
end

section: links
	# larry is to the right of moe and curly is above moe
	moe:
		right = larry
		up    = curly

	# moe is to the left of larry and curly is above larry.
	# note that curly is above both moe and larry and moe
	# and larry have a symmetric connection (they're in
	# opposite directions of each other).
	larry:
		left  = moe
		up    = curly

	# larry is below curly.  if you move up from moe and then
	# down, you'll end up on larry.
	curly:
		down  = larry
end

# The aliases section is to map the full names of the computers to their logical names used in the screens section
# One way to find the actual name of a comptuer is to run hostname from a command window
section: aliases
	# Laptop is actually known as John-Smiths-MacBook-3.local
	desktop2:
		John-Smiths-MacBook-3.local
end
```
