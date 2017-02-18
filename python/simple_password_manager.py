import os
import sys

import base64

from Crypto import Random
from Crypto.Cipher import AES
from Crypto.PublicKey import RSA
from Crypto.Hash import SHA
from Crypto.Hash import SHA256

## needs to be imported from hashlib, libcrypto
## versions do not have a block_size member var
from hashlib import sha1 as HMAC_HASH
from hmac import HMAC as HMAC_FUNC

try:
	from Crypto.Cipher import PKCS1_OAEP as RSA_PAD_SCHEME
except ImportError:
	RSA_PAD_SCHEME = None
try:
	from Crypto.Signature import PKCS1_v1_5 as RSA_SGN_SCHEME
except ImportError:
	RSA_SGN_SCHEME = None

## needed because RSAobj::operator== fails on None
RSA_NULL_KEY_OBJ = RSA._RSAobj(None, None)



AES_KEY_BIT_SIZE = 32 * 8
AES_KEY_DIR_NAME = "./"
AES_RAW_KEY_FILE = "aes_key.dat"
AES_MSG_PAD_SIZE = 64
RSA_KEY_BIT_SIZE = 8192 / 8
RSA_KEY_FMT_NAME = "PEM"
RSA_KEY_DIR_NAME = "./"
RSA_PUB_KEY_FILE = "rsa_pub_key.pem"
RSA_PRI_KEY_FILE = "rsa_pri_key.pem"
USR_DB_FILE_NAME = "user_db.dat"
PWD_DB_FILE_NAME = "pwrd_db.dat"

UNICODE_ENCODING = "utf-8"

PWRD_HASH_ROUNDS = 1024 ## HASH^n
USR_DB_SALT_SIZE =   16 ## bytes
MIN_AES_KEY_SIZE =   16 ## bytes
MIN_PASSWORD_LEN =    8 ## bytes

## hashlib.sha{1,256}
SHA160_HASH_FUNC = SHA.new
SHA256_HASH_FUNC = SHA256.new

GLOBAL_RAND_POOL = Random.new()


def null_encode_func(s): return s
def null_decode_func(s): return s

def safe_decode(s, decode_func = base64.b64decode):
	try:
		r = decode_func(s)
	except:
		## if <s> is not a base64-encoded string, then
		## it probably contains plaintext (UTF-8) data
		r = s

	return r


def verify_message_auth_code(our_mac, msg_mac, ses_key):
	## two rounds closes a timing side-channel
	msg_mac = HMAC_FUNC(ses_key, msg_mac, HMAC_HASH)
	our_mac = HMAC_FUNC(ses_key, our_mac, HMAC_HASH)
	msg_mac = msg_mac.digest()
	our_mac = our_mac.digest()
	num_val = 0

	if (len(msg_mac) != len(our_mac)):
		return False

	## fixed linear-time comparison closes another
	for i in xrange(len(our_mac)):
		num_val += (our_mac[i] == msg_mac[i])

	return (num_val == len(our_mac))


def test_user_pwrd(user_data, user_pwrd, hash_func = SHA256_HASH_FUNC):
	user_salt = user_data[1]
	user_hash = hash_func(user_pwrd + user_salt)

	for i in xrange(PWRD_HASH_ROUNDS):
		user_hash = hash_func(user_hash.digest() + user_salt)

	return (user_data[0] == user_hash.digest())

def gen_user_hash_salt(user_pass, user_salt, rand_pool = GLOBAL_RAND_POOL):
	def gen_user_salt(rand_pool, num_salt_bytes = USR_DB_SALT_SIZE):
		return (rand_pool.read(num_salt_bytes))

	def gen_user_hash(user_pwrd, user_salt,  hash_func = SHA256_HASH_FUNC):
		user_hash = hash_func(user_pwrd + user_salt)

		## apply simple stretching KDF (makes brute-force
		## attempts more time-consuming, but still is only
		## a poor-man's bcrypt)
		for i in xrange(PWRD_HASH_ROUNDS):
			user_hash = hash_func(user_hash.digest() + user_salt)

		return (user_hash.digest())

	if (len(user_salt) == 0):
		user_salt = gen_user_salt(rand_pool)

	user_hash = gen_user_hash(user_pass, user_salt)
	return (user_hash, user_salt)


def int32_to_str(n):
	assert(n >= (0      ))
	assert(n <  (1 << 32))

	s = ""
	s += "%c" % ((n >>  0) & 0xff)
	s += "%c" % ((n >>  8) & 0xff)
	s += "%c" % ((n >> 16) & 0xff)
	s += "%c" % ((n >> 24) & 0xff)

	return s

def str_to_int32(s):
	n = 0
	n += (ord(s[0]) <<  0)
	n += (ord(s[1]) <<  8)
	n += (ord(s[2]) << 16)
	n += (ord(s[3]) << 24)
	return n


def pad_str(msg, bs):
	## append <num> characters of value <num>
	num = bs - (len(msg) % bs)
	ext = num * chr(num)
	return (msg + ext)

def unpad_str(msg, bs):
	## take value of last character as indicator for
	# how many to strip off to restore the un-padded
	## plaintext
	idx = len(msg) - 1
	cnt = ord(msg[idx: ])
	return msg[0: -cnt]


def read_file(file_name, file_mode):
	try:
		f = open(file_name, file_mode)
		s = f.read()
		f = f.close()
		return s
	except IOError:
		pass

	return ""

def write_file(file_name, file_mode, file_data):
	try:
		f = open(file_name, file_mode)
		os.fchmod(f.fileno(), 0600)
		f.write("%s" % file_data)
		f = f.close()
	except IOError:
		pass



class simple_file:
	def __init__(self, file_name, file_mode, file_data = ""):
		assert(type(file_name) == str)
		assert(type(file_mode) == str)
		assert(type(file_data) == str)

		assert((file_mode.count('r') + file_mode.count('w')) <= 1)

		self.file_name = file_name
		self.file_mode = file_mode
		self.file_data = file_data

	def get_name(self): return self.file_name
	def get_mode(self): return self.file_mode
	def get_data(self): return self.file_data
	def set_data(self, data): self.file_data = data

	## transparently handles data {en,de}cryption
	def serialize_data(self, cipher_obj = None):
		if (self.get_mode()[0] == 'r'):
			assert(len(self.get_data()) == 0)
			return (self.read_data(cipher_obj))
		else:
			assert(len(self.get_data()) != 0)
			return (self.write_data(cipher_obj))


	def read_data(self, cipher_obj):
		if (not os.path.isfile(self.get_name())):
			return False
		if (cipher_obj != None):
			## always read in binary mode when decrypting
			bmode = self.get_mode() + "b"
			bytes = read_file(self.get_name(), bmode)

			self.set_data(cipher_obj.decode_decrypt_bytes(bytes, null_decode_func))
			return True

		self.set_data(read_file(self.get_name(), self.get_mode()))
		return True

	def write_data(self, cipher_obj):
		if (os.path.isfile(self.get_name())):
			return False
		if (cipher_obj != None):
			## always write in binary mode when encrypting
			bmode = self.get_mode() + "b"
			bytes = cipher_obj.encrypt_encode_bytes(self.get_data(), null_encode_func)

			write_file(self.get_name(), bmode, bytes)
			return True

		write_file(self.get_name(), self.get_mode(), self.get_data())
		return True



class rsa_cipher:
	def __init__(self, key_dir = RSA_KEY_DIR_NAME):
		self.set_rnd_gen(Random.new())
		self.set_instance_keys(key_dir)
		self.set_pad_scheme(RSA_PAD_SCHEME)
		self.set_sgn_scheme(RSA_SGN_SCHEME)

	def set_rnd_gen(self, rnd_gen): self.rnd_gen = rnd_gen
	def set_pub_key(self, pub_key): self.pub_key = pub_key
	def set_pri_key(self, pri_key): self.pri_key = pri_key

	def get_pub_key(self): return self.pub_key
	def get_pri_key(self): return self.pri_key

	def sanity_test_keys(self):
		pk = (self.pri_key.publickey())
		b0 = (pk == self.pub_key)
		b1 = (pk.exportKey(RSA_KEY_FMT_NAME) == self.pub_key.exportKey(RSA_KEY_FMT_NAME))
		b2 = ((not self.pub_key.has_private()) and self.pri_key.has_private())
		return (b0 and b1 and b2)

	def set_pad_scheme(self, scheme):
		if (scheme == None):
			self.enc_pad_scheme = None
			self.dec_pad_scheme = None
		else:
			self.enc_pad_scheme = scheme.new(self.pub_key)
			self.dec_pad_scheme = scheme.new(self.pri_key)
	def set_sgn_scheme(self, scheme):
		if (scheme == None):
			self.msg_sign_scheme = None
			self.msg_auth_scheme = None
		else:
			self.msg_sign_scheme = scheme.new(self.pri_key)
			self.msg_auth_scheme = scheme.new(self.pub_key)

	def set_instance_keys(self, key_dir):
		if (key_dir == None):
			self.set_pub_key(RSA_NULL_KEY_OBJ)
			self.set_pri_key(RSA_NULL_KEY_OBJ)
			return

		if (not self.import_keys(key_dir)):
			self.generate_keys()

		assert(self.sanity_test_keys())

	def generate_keys(self, num_bits = RSA_KEY_BIT_SIZE):
		self.set_pri_key(RSA.generate(num_bits, self.rnd_gen.read))
		self.set_pub_key(self.pri_key.publickey())
		return True


	def import_key(self, key_str):
		return (RSA.importKey(key_str))

	def import_keys(self, key_dir):
		assert(len(key_dir) == 0 or key_dir[-1] == '/')

		pub_key_str = read_file(key_dir + RSA_PUB_KEY_FILE, "r")
		pri_key_str = read_file(key_dir + RSA_PRI_KEY_FILE, "r")

		## for convenience, insist that both files exist
		if (len(pub_key_str) != 0 and len(pri_key_str) != 0):
			self.set_pub_key(self.import_key(pub_key_str))
			self.set_pri_key(self.import_key(pri_key_str))
			return True

		return False

	def export_keys(self, key_dir):
		assert(len(key_dir) != 0)
		assert(key_dir[-1] == '/')

		if (not os.path.isdir(key_dir)):
			os.mkdir(key_dir, 0700)

		write_file(key_dir + RSA_PUB_KEY_FILE, "w", self.pub_key.exportKey(RSA_KEY_FMT_NAME))
		write_file(key_dir + RSA_PRI_KEY_FILE, "w", self.pri_key.exportKey(RSA_KEY_FMT_NAME))


	## these make sure that any native unicode inputs are converted
	## to standard (UTF-8 encoded) strings, since crypto operations
	## are only defined for raw inputs
	##
	## "\xe2\x98\x83".decode("utf-8") == u"\u2603"
	## u"\u2603".encode("utf-8") == "\xe2\x98\x83"
	##
	def encrypt_encode_bytes_utf8(self, raw_bytes, encode_func = base64.b64encode):
		return (self.encrypt_encode_bytes(raw_bytes.encode(UNICODE_ENCODING), encode_func))
	def decode_decrypt_bytes_utf8(self, enc_bytes, decode_func = base64.b64decode):
		return (self.decode_decrypt_bytes(enc_bytes.encode(UNICODE_ENCODING), decode_func))

	## encrypt then encode; performs C=ENCODE(ENCRYPT_RSA(M))
	def encrypt_encode_bytes(self, raw_bytes, encode_func = base64.b64encode):
		assert(type(raw_bytes) == str)
		assert(len(raw_bytes) != 0)
		assert(self.pub_key.size() >= (len(raw_bytes) * 8))
		assert(ord(raw_bytes[0]) != 0)

		if (self.enc_pad_scheme != None):
			enc_bytes = self.enc_pad_scheme.encrypt(raw_bytes)
		else:
			## NOTE: RSAobj.encrypt() returns a tuple (!)
			enc_bytes = self.pub_key.encrypt(raw_bytes, "")[0]

		return (encode_func(enc_bytes))

	## decode then decrypt; performs M=DECRYPT_RSA(DECODE(C))
	def decode_decrypt_bytes(self, enc_bytes, decode_func = base64.b64decode):
		assert(type(enc_bytes) == str)
		assert(len(enc_bytes) != 0)
		assert((self.pri_key.size() + 1) == (len(decode_func(enc_bytes)) * 8))

		enc_bytes = decode_func(enc_bytes)

		if (self.dec_pad_scheme != None):
			dec_bytes = self.dec_pad_scheme.decrypt(enc_bytes)
		else:
			dec_bytes = self.pri_key.decrypt(enc_bytes)[0]

		return dec_bytes


	def sign_bytes_utf8(self, msg_bytes):
		return (self.sign_bytes(msg_bytes.encode(UNICODE_ENCODING)))
	def auth_bytes_utf8(self, msg_bytes, sig_bytes):
		return (self.auth_bytes(msg_bytes.encode(UNICODE_ENCODING), sig_bytes))

	def sign_bytes(self, msg_bytes):
		assert(type(msg_bytes) == str)
		assert(len(msg_bytes) != 0)

		msg_bytes = SHA256_HASH_FUNC(msg_bytes)

		if (self.msg_sign_scheme != None):
			## scheme.sign() expects an object from Crypto.Hash
			ret = self.msg_sign_scheme.sign(msg_bytes)
		else:
			## RSAobj.sign() returns a tuple
			ret = str(self.pri_key.sign(msg_bytes.digest(), "")[0])

		assert(type(ret) == str)
		return ret

	def auth_bytes(self, msg_bytes, sig_bytes):
		assert(type(msg_bytes) == str)
		assert(type(sig_bytes) == str)
		assert(len(msg_bytes) != 0)

		msg_bytes = SHA256_HASH_FUNC(msg_bytes)

		if (self.msg_auth_scheme != None):
			## scheme.verify() expects an object from Crypto.Hash
			ret = self.msg_auth_scheme.verify(msg_bytes, sig_bytes)
		else:
			## RSAobj.verify() expects a tuple
			ret = (self.pub_key.verify(msg_bytes.digest(), (long(sig_bytes), 0L)))

		assert(type(ret) == bool)
		return ret




class aes_cipher:
	def __init__(self, key_dir = AES_KEY_DIR_NAME, pad_length = AES_MSG_PAD_SIZE):
		assert(type(key_dir) == str)
		assert((pad_length % 16) == 0)

		## add padding to message s.t. length is a multiple of this
		## use digest of raw key as actual key (so length is always
		## correct)
		self.pad_length = pad_length
		self.random_gen = Random.new()
		self.khash_func = SHA256_HASH_FUNC

		## initialize key from file if possible; user can override
		## (by calling set_key(generate_key(...)) with any desired
		## raw string)
		self.set_instance_key(key_dir)


	def set_instance_key(self, key_dir):
		if (not self.import_key(key_dir)):
			self.set_key(self.generate_key(""))


	def generate_key(self, raw_key, key_len = AES_KEY_BIT_SIZE):
		## if key-string is empty, generate one randomly
		## otherwise hash the raw string and take digest
		##
		## sha256 digests are a convenient 32 bytes long
		## (256 bits, matches the AES standard key-size)
		if (len(raw_key) == 0):
			key_str = self.random_gen.read(key_len / 8)
			key_str = self.khash_func(key_str)
		else:
			key_str = self.khash_func(raw_key)

		return (key_str.digest())

	def get_key(self): return self.key_string
	def set_key(self, s): self.key_string = s


	def import_key(self, key_dir):
		assert(len(key_dir) == 0 or key_dir[-1] == '/')

		## read the (binary) hashed key-string
		key_str = read_file(key_dir + AES_RAW_KEY_FILE, "rb")

		if (len(key_str) != 0):
			self.set_key(key_str)
			return True

		return False

	def export_key(self, key_dir):
		assert(len(key_dir) != 0)
		assert(key_dir[-1] == '/')

		if (not os.path.isdir(key_dir)):
			os.mkdir(key_dir, 0700)

		write_file(key_dir + AES_RAW_KEY_FILE, "wb", self.get_key())


	def encrypt_encode_bytes_utf8(self, raw_bytes, encode_func = base64.b64encode):
		return (self.encrypt_encode_bytes(raw_bytes.encode(UNICODE_ENCODING), encode_func))
	def decode_decrypt_bytes_utf8(self, enc_bytes, decode_func = base64.b64decode):
		return (self.decode_decrypt_bytes(enc_bytes.encode(UNICODE_ENCODING), decode_func))

	## encrypt then encode; performs C=ENCODE(ENCRYPT_AES(M))
	## note: adding time-stamps is responsibility of caller
	def encrypt_encode_bytes(self, raw_bytes, encode_func = base64.b64encode):
		assert(type(raw_bytes) == str)
		assert(len(raw_bytes) != 0)

		## use a different IV for each message encryption
		ini_vector = self.random_gen.read(AES.block_size)
		aes_object = AES.new(self.key_string, AES.MODE_CBC, ini_vector)

		pad_bytes = pad_str(raw_bytes, self.pad_length)
		enc_bytes = aes_object.encrypt(pad_bytes)

		## encoding step is optional (e.g. if writing to file)
		return (encode_func(ini_vector + enc_bytes))

	## decode then decrypt; performs M=DECRYPT_AES(DECODE(C))
	def decode_decrypt_bytes(self, enc_bytes, decode_func = base64.b64decode):
		assert(type(enc_bytes) == str)
		assert(len(enc_bytes) != 0)

		enc_bytes = decode_func(enc_bytes)

		ini_vector = enc_bytes[0: AES.block_size]
		aes_object = AES.new(self.key_string, AES.MODE_CBC, ini_vector)

		dec_bytes = aes_object.decrypt(enc_bytes[AES.block_size: ])
		dec_bytes = unpad_str(dec_bytes, self.pad_length)
		return dec_bytes


	def encrypt_sign_bytes_utf8(self, raw_msg, encode_func = base64.b64encode):
		return (self.encrypt_sign_bytes(raw_msg.encode(UNICODE_ENCODING), encode_func))
	def auth_decrypt_bytes_utf8(self, (enc_msg, msg_mac), decode_func = base64.b64decode):
		return (self.auth_decrypt_bytes((enc_msg.encode(UNICODE_ENCODING), msg_mac.encode(UNICODE_ENCODING)), decode_func))

	def encrypt_sign_bytes(self, raw_msg, encode_func = base64.b64encode):
		assert(type(raw_msg) == str)

		## encrypt, then sign (HMAC = H((K ^ O) | H((K ^ I) | M)))
		enc_msg = self.encrypt_encode_bytes(raw_msg, null_encode_func)
		msg_mac = HMAC_FUNC(self.get_key(), enc_msg, HMAC_HASH)
		msg_mac = encode_func(msg_mac.digest())
		enc_msg = encode_func(enc_msg)

		return (enc_msg, msg_mac)

	def auth_decrypt_bytes(self, (enc_msg, msg_mac), decode_func = base64.b64decode):
		assert(type(enc_msg) == str)
		assert(type(msg_mac) == str)

		## auth, then decrypt
		msg_mac = decode_func(msg_mac)
		enc_msg = decode_func(enc_msg)
		our_mac = HMAC_FUNC(self.get_key(), enc_msg, HMAC_HASH)
		our_mac = our_mac.digest()

		if (verify_message_auth_code(our_mac, msg_mac, self.get_key())):
			return (self.decode_decrypt_bytes(enc_msg, null_decode_func))

		## empty string counts as false and can not be
		## returned from a normal decryption operation
		## (since encryption requires non-empty inputs)
		return ""




## represents e.g. a client's password table
## this is NEVER stored in un-encrypted form
class pwrd_database:
	def __init__(self, aes_obj):
		assert(aes_obj != None)

		self.pwrd_db = {}
		self.aes_obj = aes_obj


	def export_to_file(self, file_name):
		f = simple_file(file_name, "w", self.export_to_string())
		r = f.serialize_data(self.aes_obj)
		return r

	def import_from_file(self, file_name):
		f = simple_file(file_name, "r")
		r = f.serialize_data(self.aes_obj)
		self.import_from_string(f.get_data())
		return r


	def export_to_string(self):
		## if DB is empty, string will just contain this int32
		s = int32_to_str(self.get_num_pwrds())

		for (pwrd_name, pwrd_list) in self.pwrd_db.items():
			assert(type(pwrd_list) == list)

			s += int32_to_str(len(pwrd_name))
			s += int32_to_str(len(pwrd_list))
			s += ("%s" % pwrd_name)

			for pwrd_data in pwrd_list:
				assert(type(pwrd_data) == tuple)

				s += int32_to_str(len(pwrd_data[0]))
				s += int32_to_str(len(pwrd_data[1]))
				s += int32_to_str(len(pwrd_data[2]))
				s += ("%s" % pwrd_data[0])
				s += ("%s" % pwrd_data[1])
				s += ("%s" % pwrd_data[2])

		return s

	def import_from_string(self, s):
		## can not import from an empty string
		if (len(s) == 0):
			return False
		## can not import into a non-empty DB
		## (aside from merging which is messy)
		if (self.get_num_pwrds() != 0):
			return False

		assert(s != self.export_to_string())

		str_index = 4
		num_pwrds = str_to_int32(s[0: str_index])

		for n in xrange(num_pwrds):
			i = str_to_int32(s[str_index +  0: str_index +  4]) ## len(pwrd_name)
			j = str_to_int32(s[str_index +  4: str_index +  8]) ## len(pwrd_list)
			k = (4 * 2)

			pwrd_name = s[str_index + k: str_index + k + i]
			str_index += (k + i)

			for m in xrange(j):
				i = str_to_int32(s[str_index + 0: str_index +  4]) ## len(user_name)
				j = str_to_int32(s[str_index + 4: str_index +  8]) ## len(user_pwrd)
				k = str_to_int32(s[str_index + 8: str_index + 12]) ## len(user_text)
				h = (4 * 3)

				user_name = s[str_index + h + 0 + 0: str_index + h + i + 0 + 0]
				user_pwrd = s[str_index + h + 0 + i: str_index + h + i + j + 0]
				user_text = s[str_index + h + i + j: str_index + h + i + j + k]
				str_index += (h + i + j + k)

				self.insert_pwrd(pwrd_name, user_name, user_pwrd, user_text)

		return True


	def enumerate_pwrds(self, encode_func = null_encode_func):
		pwrd_names = self.pwrd_db.keys()
		pwrd_names.sort()

		print("[enum_pwrds]")
		for pwrd_name in pwrd_names:
			pwrd_list = self.get_pwrd_list(pwrd_name)

			for pwrd_data in pwrd_list:
				user_name = pwrd_data[0]
				user_pwrd = pwrd_data[1]
				user_text = pwrd_data[2]
				print("\tname=\"%s\": (user=\"%s\" pwrd=\"%s\" text=\"%s\")" % (pwrd_name, user_name, encode_func(user_pwrd), user_text))

	def get_num_pwrds(self): return (len(self.pwrd_db))
	def has_pwrd_list(self, pwrd_name): return self.pwrd_db.has_key(pwrd_name)
	def get_pwrd_list(self, pwrd_name): return self.pwrd_db[pwrd_name]


	## db["pwrd_name"] = ("user_name", "user_pwrd", "user_text")
	## note: can also be used to update existing entries
	def insert_pwrd(self, pwrd_name, user_name, user_pwrd, user_text = ""):
		assert(type(pwrd_name) == str)
		assert(type(user_name) == str)
		assert(type(user_pwrd) == str)
		assert(type(user_text) == str)

		if (not self.has_pwrd_list(pwrd_name)):
			self.pwrd_db[pwrd_name]  = [(user_name, user_pwrd, user_text)]
		else:
			self.pwrd_db[pwrd_name] += [(user_name, user_pwrd, user_text)]

	def delete_pwrd(self, pwrd_name):
		if (not self.has_pwrd_list(pwrd_name)):
			return False

		## delete all elements in the pwrd-list
		del self.pwrd_db[pwrd_name]
		return True


## represents e.g. a (web-)server's user table
## this is OPTIONALLY stored in encrypted form
class user_database:
	def __init__(self, aes_obj = None, hsh_fun = SHA256_HASH_FUNC):
		self.user_db = {}
		self.rnd_gen = Random.new()
		self.aes_obj = aes_obj
		self.hsh_fun = hsh_fun

	def enumerate_users(self, encode_func = base64.b64encode):
		user_names = self.user_db.keys()
		user_names.sort()

		print("[enum_users]")
		for user_name in user_names:
			user_data = self.get_user_data(user_name)
			user_hash = user_data[0]
			user_salt = user_data[1]
			print("\tname=\"%s\": (hash=\"%s\" salt=\"%s\")" % (user_name, encode_func(user_hash), encode_func(user_salt)))

	def get_num_users(self): return (len(self.user_db))
	def has_user_data(self, user_name): return (self.user_db.has_key(user_name))
	def get_user_data(self, user_name): return (self.user_db[user_name])


	def export_to_sql(self): pass
	def import_from_sql(self): pass


	def export_to_file(self, file_name):
		f = simple_file(file_name, "w", self.export_to_string())
		r = f.serialize_data(self.aes_obj)
		return r

	def import_from_file(self, file_name):
		f = simple_file(file_name, "r")
		r = f.serialize_data(self.aes_obj)
		self.import_from_string(f.get_data())
		return r


	def export_to_string(self):
		## if DB is empty, string will just contain this int32
		s = int32_to_str(self.get_num_users())

		for (user_name, user_data) in self.user_db.items():
			assert(type(user_data) == tuple)

			s += int32_to_str(len(user_name))
			s += int32_to_str(len(user_data[0]))
			s += int32_to_str(len(user_data[1]))
			s += ("%s" % user_name)
			s += ("%s" % user_data[0])
			s += ("%s" % user_data[1])

		return s

	def import_from_string(self, s):
		## can not import from an empty string
		if (len(s) == 0):
			return False
		## can not import into a non-empty DB
		## (aside from merging which is messy)
		if (self.get_num_users() != 0):
			return False

		assert(s != self.export_to_string())

		str_index = 4
		num_users = str_to_int32(s[0: str_index])

		for n in xrange(num_users):
			i = str_to_int32(s[str_index + 0: str_index +  4]) ## len(user_name)
			j = str_to_int32(s[str_index + 4: str_index +  8]) ## len(user_hash)
			k = str_to_int32(s[str_index + 8: str_index + 12]) ## len(user_salt)

			str_index += (4 * 3)
			user_name = s[str_index + (0 + 0): str_index + (i + 0 + 0)]
			user_hash = s[str_index + (i + 0): str_index + (i + j + 0)]
			user_salt = s[str_index + (i + j): str_index + (i + j + k)]
			str_index += (i + j + k)

			self.insert_user(user_name, (user_hash, user_salt))

		return True


	def insert_user(self, user_name, user_data):
		assert(not self.has_user_data(user_name))
		assert(type(user_data) == tuple)
		assert(type(user_data[0]) == str)
		assert(type(user_data[1]) == str)
		self.user_db[user_name] = user_data

	def delete_user(self, user_name):
		if (self.has_user_data(user_name)):
			del self.user_db[user_name]
			return True

		return False

	def rename_user(self, cur_user_name, new_user_name):
		if (new_user_name == cur_user_name):
			return False
		if (not self.has_user_data(cur_user_name)):
			return False
		if (self.has_user_data(new_user_name)):
			return False

		self.insert_user(new_user_name, self.user_db[cur_user_name])
		self.delete_user(cur_user_name)

	def update_user(self, user_name, cur_user_pwrd, new_user_pwrd):
		if (not self.authenticate_user(user_name, cur_user_pwrd)):
			return False

		## construct a (hash(pass + salt), salt) tuple for a user
		## (prefer generating a new salt to re-using the old one)
		##
		self.user_db[user_name] = gen_user_hash_salt(new_user_pwrd, "", self.rnd_gen)
		return True


	## note: these assume client->server comms are encrypted
	## (at least registration / authentication / update / ..)
	##
	## salt can optionally be sent by client, but preferably
	## let the server control the process of generating them
	def register_user(self, user_name, user_pwrd, user_salt = ""):
		assert(type(user_name) == str)
		assert(type(user_pwrd) == str)
		assert(type(user_salt) == str)

		if (self.has_user_data(user_name)):
			return False

		self.insert_user(user_name, gen_user_hash_salt(user_pwrd, user_salt, self.rnd_gen))
		return True

	def authenticate_user(self, user_name, user_pwrd):
		assert(type(user_name) == str)
		assert(type(user_pwrd) == str)

		if (not self.has_user_data(user_name)):
			return False

		return (test_user_pwrd(self.user_db[user_name], user_pwrd, self.hsh_fun))




def parse_args(argc, argv):
	arg_params = {
		"aes_key_str": "", ## overrides any key in aes_key_dir if non-empty
		"aes_key_dir": "", ## key will be randomly generated if empty/missing
		"rsa_key_dir": "", ## keys will be randomly generated if empty/missing
		"enc_user_db":  1, ## whether to encrypt the user-database on export
		"new_user_db":  1, ## whether to create a new user-database
		"new_pwrd_db":  1, ## whether to create a new pwrd-database
	}

	try:
		for i in xrange(argc - 1):
			if (argv[i] == "--aks"): arg_params["aes_key_str"] =    (argv[i + 1]); continue
			if (argv[i] == "--akd"): arg_params["aes_key_dir"] =    (argv[i + 1]); continue
			if (argv[i] == "--rkd"): arg_params["rsa_key_dir"] =    (argv[i + 1]); continue
			if (argv[i] == "--eud"): arg_params["enc_user_db"] = int(argv[i + 1]); continue
			if (argv[i] == "--nud"): arg_params["new_user_db"] = int(argv[i + 1]); continue
			if (argv[i] == "--npd"): arg_params["new_pwrd_db"] = int(argv[i + 1]); continue
	except ValueError:
		pass

	return arg_params


def create_or_import_pwrd_database(arg_params, aes_object):
	pwrd_db = pwrd_database(aes_object)

	if (arg_params["new_pwrd_db"] != 0):
		pwrd_db.insert_pwrd("site1", "user1", "123455", "user1@site1")
		pwrd_db.insert_pwrd("site1", "user2", "123457", "user2@site1")
		pwrd_db.insert_pwrd("site2", "user3", "778899", "user3@site2")
		pwrd_db.insert_pwrd("site2", "user4", "778899", "user4@site2")

		pwrd_db.enumerate_pwrds()
		pwrd_db.export_to_file(PWD_DB_FILE_NAME)
	else:
		if (pwrd_db.import_from_file(PWD_DB_FILE_NAME)):
			pwrd_db.enumerate_pwrds()

			assert(pwrd_db.has_pwrd_list("site1"))
			assert(pwrd_db.has_pwrd_list("site2"))
			assert(len(pwrd_db.get_pwrd_list("site1")) == 2)
			assert(len(pwrd_db.get_pwrd_list("site2")) == 2)

def create_or_import_user_database(arg_params, aes_object):
	user_db = user_database(((arg_params["enc_user_db"] != 0) and aes_object) or None)

	if (arg_params["new_user_db"] != 0):
		user_db.register_user("user1", "pwrd1")
		user_db.register_user("user2", "pwrd2")

		user_db.enumerate_users()
		user_db.export_to_file(USR_DB_FILE_NAME)

		assert(user_db.authenticate_user("user1", "pwrd1"))
		assert(user_db.authenticate_user("user2", "pwrd2"))
	else:
		if (user_db.import_from_file(USR_DB_FILE_NAME)):
			user_db.enumerate_users()

			assert(user_db.authenticate_user("user1", "pwrd1"))
			assert(user_db.authenticate_user("user2", "pwrd2"))
			assert(not user_db.authenticate_user("user2", "pwrD2"))
			assert(not user_db.authenticate_user("User2", "pwrd2"))


def main(argc, argv):
	arg_params = parse_args(argc - 1, argv[1: ])
	aes_object = aes_cipher(arg_params["aes_key_dir"])
	rsa_object = rsa_cipher(arg_params["rsa_key_dir"])

	## check for user-specified key override
	if (len(arg_params["aes_key_str"]) != 0):
		aes_object.set_key(aes_object.generate_key(arg_params["aes_key_str"]))

	aes_object.export_key(AES_KEY_DIR_NAME)
	rsa_object.export_keys(RSA_KEY_DIR_NAME)

	assert(str_to_int32(int32_to_str(123456789)) == 123456789)
	assert(rsa_object.auth_bytes("12345", rsa_object.sign_bytes("12345")))
	assert(aes_object.auth_decrypt_bytes(aes_object.encrypt_sign_bytes("squeamish ossifrage")))

	create_or_import_pwrd_database(arg_params, aes_object)
	create_or_import_user_database(arg_params, aes_object)
	return 0

sys.exit(main(len(sys.argv), sys.argv))

