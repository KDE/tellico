#!/usr/bin/env python
# -*- coding: utf-8 -*-

# ***************************************************************************
#    Copyright (C) 2006-2009 Mathias Monnerville <tellico@monnerville.com>
# ***************************************************************************
#
# ***************************************************************************
# *                                                                         *
# *   This program is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU General Public License as        *
# *   published by the Free Software Foundation; either version 2 of        *
# *   the License or (at your option) version 3 or any later version        *
# *   accepted by the membership of KDE e.V. (or its successor approved     *
# *   by the membership of KDE e.V.), which shall act as a proxy            *
# *   defined in Section 14 of version 3 of the license.                    *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU General Public License for more details.                          *
# *                                                                         *
# *   You should have received a copy of the GNU General Public License     *
# *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
# *                                                                         *
# ***************************************************************************

# $Id: comics_darkhorsecomics.py 123 2006-03-24 08:47:48Z mathias $

"""
This script has to be used with tellico (http://periapsis.org/tellico) as an external data source program.
It allows searching through the Dark Horse Comics web database.

Related info and cover are fetched automatically. It takes only one argument (comic title).

Tellico data source setup:
- source name: Dark Horse Comics (US) (or whatever you want :)
- Collection type: comics collection
- Result type: tellico
- Path: /path/to/script/comics_darkhorsecomics.py
- Arguments:
Title (checked) = %1
Update (checked) = %{title}
"""

import sys, os, re, md5, random, string
import urllib, urllib2, time, base64
import xml.dom.minidom

XML_HEADER = """<?xml version="1.0" encoding="UTF-8"?>"""
DOCTYPE = """<!DOCTYPE tellico PUBLIC "-//Robby Stephenson/DTD Tellico V9.0//EN" "http://periapsis.org/tellico/dtd/v9/tellico.dtd">"""
NULLSTRING = ''

VERSION = "0.2"


def genMD5():
	"""
	Generates and returns a random md5 string. Its main purpose is to allow random
	image file name generation.
	"""
	obj = md5.new()
	float = random.random()
	obj.update(str(float))
	return obj.hexdigest()

class BasicTellicoDOM:
	"""
	This class manages tellico's XML data model (DOM)
	"""
	def __init__(self):
		self.__doc = xml.dom.minidom.Document()
		self.__root = self.__doc.createElement('tellico')
		self.__root.setAttribute('xmlns', 'http://periapsis.org/tellico/')
		self.__root.setAttribute('syntaxVersion', '9')

		self.__collection = self.__doc.createElement('collection')
		self.__collection.setAttribute('title', 'My Comics')
		self.__collection.setAttribute('type', '6')

		self.__images = self.__doc.createElement('images')

		self.__root.appendChild(self.__collection)
		self.__doc.appendChild(self.__root)

		# Current movie id. See entry's id attribute in self.addEntry()
		self.__currentId = 0


	def addEntry(self, movieData):
		"""
		Add a comic entry.
		Returns an entry node instance
		"""
		d = movieData
		entryNode = self.__doc.createElement('entry')
		entryNode.setAttribute('id', str(self.__currentId))

		titleNode = self.__doc.createElement('title')
		titleNode.appendChild(self.__doc.createTextNode(unicode(d['title'], 'latin-1').encode('utf-8')))

		yearNode = self.__doc.createElement('pub_year')
		yearNode.appendChild(self.__doc.createTextNode(d['pub_year']))

		countryNode = self.__doc.createElement('country')
		countryNode.appendChild(self.__doc.createTextNode(d['country']))
		pubNode = self.__doc.createElement('publisher')
		pubNode.appendChild(self.__doc.createTextNode(d['publisher']))
		langNode = self.__doc.createElement('language')
		langNode.appendChild(self.__doc.createTextNode(d['language']))

		writersNode = self.__doc.createElement('writers')
		for g in d['writer']:
			writerNode = self.__doc.createElement('writer')
			writerNode.appendChild(self.__doc.createTextNode(unicode(g, 'latin-1').encode('utf-8')))
			writersNode.appendChild(writerNode)

		genresNode = self.__doc.createElement('genres')
		for g in d['genre']:
			genreNode = self.__doc.createElement('genre')
			genreNode.appendChild(self.__doc.createTextNode(unicode(g, 'latin-1').encode('utf-8')))
			genresNode.appendChild(genreNode)

		commentsNode = self.__doc.createElement('comments')
		#for g in d['comments']:
		#	commentsNode.appendChild(self.__doc.createTextNode(unicode("%s\n\n" % g, 'latin-1').encode('utf-8')))
		commentsData = string.join(d['comments'], '\n\n')
		commentsNode.appendChild(self.__doc.createTextNode(unicode(commentsData, 'latin-1').encode('utf-8')))

		artistsNode = self.__doc.createElement('artists')
		for k, v in d['artist'].iteritems():
			artistNode = self.__doc.createElement('artist')
			artistNode.appendChild(self.__doc.createTextNode(unicode(v, 'latin-1').encode('utf-8')))
			artistsNode.appendChild(artistNode)

		pagesNode = self.__doc.createElement('pages')
		pagesNode.appendChild(self.__doc.createTextNode(d['pages']))

		issueNode = self.__doc.createElement('issue')
		issueNode.appendChild(self.__doc.createTextNode(d['issue']))

		if d['image']:
			imageNode = self.__doc.createElement('image')
			imageNode.setAttribute('format', 'JPEG')
			imageNode.setAttribute('id', d['image'][0])
			imageNode.appendChild(self.__doc.createTextNode(unicode(d['image'][1], 'latin-1').encode('utf-8')))

			coverNode = self.__doc.createElement('cover')
			coverNode.appendChild(self.__doc.createTextNode(d['image'][0]))

		for name in (	'writersNode', 'genresNode', 'artistsNode', 'pagesNode', 'yearNode',
						'titleNode', 'issueNode', 'commentsNode', 'pubNode', 'langNode',
						'countryNode' ):
			entryNode.appendChild(eval(name))

		if d['image']:
			entryNode.appendChild(coverNode)
			self.__images.appendChild(imageNode)

		self.__collection.appendChild(entryNode)

		self.__currentId += 1
		return entryNode

	def printEntry(self, nEntry):
		"""
		Prints entry's XML content to stdout
		"""
		try:
			print nEntry.toxml()
		except:
			print sys.stderr, "Error while outputing XML content from entry to Tellico"

	def printXMLTree(self):
		"""
		Outputs XML content to stdout
		"""
		self.__collection.appendChild(self.__images)
		print XML_HEADER; print DOCTYPE
		print self.__root.toxml()


class DarkHorseParser:
	def __init__(self):
		self.__baseURL 	 = 'http://www.darkhorse.com'
		self.__basePath  = '/profile/profile.php?sku='
		self.__searchURL = '/search/search.php?frompage=userinput&sstring=%s&x=0&y=0'
		self.__coverPath = 'http://images.darkhorse.com/covers/'
		self.__movieURL  = self.__baseURL + self.__basePath

		# Define some regexps
		self.__regExps = { 	'title' 		: '<font size="\+2"><b>(?P<title>.*?)</b></font>',
							'pub_date'		: '<b>Pub.* Date:</b> *<a.*>(?P<pub_date>.*)</a>',
							'desc'			: '<p>(?P<desc>.*?)<br>',
							'writer'		: '<b>Writer: *</b> *<a.*?>(?P<writer>.*)</a>',
							'cover_artist'	: '<b>Cover Artist: *</b> *<a.*>(?P<cover_artist>.*)</a>',
							'penciller'		: '<b>Penciller: *</b> *<a.*>(?P<penciller>.*)</a>',
							'inker'			: '<b>Inker: *</b> *<a.*>(?P<inker>.*)</a>',
							'letterer'		: '<b>Letterer: *</b> *<a.*>(?P<letterer>.*)</a>',
							'colorist'		: '<b>Colorist: *</b> *<a.*>(?P<colorist>.*)</a>',
							'genre'			: '<b>Genre: *</b> *<a.*?>(?P<genre>.*?)</a><br>',
							'format'		: '<b>Format: *</b> *(?P<format>.*?)<br>',
						}

		# Compile patterns objects
		self.__regExpsPO = {}
		for k, pattern in self.__regExps.iteritems():
			self.__regExpsPO[k] = re.compile(pattern)

		self.__domTree = BasicTellicoDOM()

	def run(self, title):
		"""
		Runs the parser: fetch movie related links, then fills and prints the DOM tree
		to stdout (in tellico format) so that tellico can use it.
		"""
		self.__getMovie(title)
		# Print results to stdout
		self.__domTree.printXMLTree()

	def __getHTMLContent(self, url):
		"""
		Fetch HTML data from url
		"""
		u = urllib2.urlopen(url)
		self.__data = u.read()
		u.close()

	def __fetchMovieLinks(self):
		"""
		Retrieve all links related to the search. self.__data contains HTML content fetched by self.__getHTMLContent()
		that need to be parsed.
		"""
		matchList = re.findall("""<a *href="%s(?P<page>.*?)">(?P<title>.*?)</a>""" % self.__basePath.replace('?', '\?'), self.__data)
		if not matchList: return None

		return matchList

	def __fetchCover(self, path, delete = True):
		"""
		Fetch cover to /tmp. Returns base64 encoding of data.
		The image is deleted if delete is True
		"""
		md5 = genMD5()
		imObj = urllib2.urlopen(path.strip())
		img = imObj.read()
		imObj.close()
		imgPath = "/tmp/%s.jpeg" % md5
		try:
			f = open(imgPath, 'w')
			f.write(img)
			f.close()
		except:
			print sys.stderr, "Error: could not write image into /tmp"

		b64data = (md5 + '.jpeg', base64.encodestring(img))

		# Delete temporary image
		if delete:
			try:
				os.remove(imgPath)
			except:
				print sys.stderr, "Error: could not delete temporary image /tmp/%s.jpeg" % md5

		return b64data

	def __fetchMovieInfo(self, url):
		"""
		Looks for movie information
		"""
		self.__getHTMLContent(url)

		# First grab picture data
		imgMatch = re.search("""<img src="%s(?P<imgpath>.*?)".*>""" % self.__coverPath, self.__data)
		if imgMatch:
			imgPath = self.__coverPath + imgMatch.group('imgpath')
			# Fetch cover and gets its base64 encoded data
			b64img = self.__fetchCover(imgPath)
		else:
			b64img = None

		# Now isolate data between <div class="bodytext">...</div> elements
		# re.S sets DOTALL; it makes the "." special character match any character at all, including a newline
		m = re.search("""<div class="bodytext">(?P<part>.*)</div>""", self.__data, re.S)
		self.__data = m.group('part')

		matches = {}
		data = {}
		data['comments'] = []
		data['artist'] = {}

		# Default values
		data['publisher'] 	= 'Dark Horse Comics'
		data['language'] 	= 'English'
		data['country'] 	= 'USA'

		data['image'] 		= b64img
		data['pub_year']	= NULLSTRING

		for name, po in self.__regExpsPO.iteritems():
			data[name] = NULLSTRING
			if name == 'desc':
				matches[name] = re.findall(self.__regExps[name], self.__data, re.S | re.I)
			else:
				matches[name] = po.search(self.__data)

			if matches[name]:
				if name == 'title':
					title = matches[name].group('title').strip()
					data[name] = title
					# Look for issue information
					m = re.search("#(?P<issue>[0-9]+)", title)
					if m:
						data['issue'] = m.group('issue')
					else:
						data['issue'] = ''

				elif name == 'pub_date':
					pub_date = matches[name].group('pub_date').strip()
					data['pub_year'] = pub_date[-4:]
					# Add this to comments field
					data['comments'].insert(0, "Pub. Date: %s" % pub_date)

				elif name == 'desc':
					# Find biggest size
					max = 0
					for i in range(len(matches[name])):
						if len(matches[name][i]) > len(matches[name][max]):
							max = i
					data['comments'].append(matches[name][max].strip())

				elif name == 'writer':
					# We may find several writers
					data[name] = []
					writersList = re.sub('</?a.*?>', '', matches[name].group('writer')).split(',')
					for d in writersList:
						data[name].append(d.strip())

				elif name == 'cover_artist':
					data['artist']['Cover Artist'] = matches[name].group('cover_artist').strip()

				elif name == 'penciller':
					data['artist']['Penciller'] = matches[name].group('penciller').strip()

				elif name == 'inker':
					data['artist']['Inker'] = matches[name].group('inker').strip()

				elif name == 'colorist':
					data['artist']['Colorist'] = matches[name].group('colorist').strip()

				elif name == 'letterer':
					data['artist']['Letterer'] = matches[name].group('letterer').strip()

				elif name == 'genre':
					# We may find several genres
					data[name] = []
					genresList = re.sub('</?a.*?>', '', matches[name].group('genre')).split(',')
					for d in genresList:
						data[name].append(d.strip())

				elif name == 'format':
					format = matches[name].group('format').strip()
					data['comments'].insert(1, format)
					m = re.search("(?P<pages>[0-9]+)", format)
					if m:
						data['pages'] = m.group('pages')
					else:
						data['pages'] = ''

		return data


	def __getMovie(self, title):
		if not len(title): return

		self.__title = title
		self.__getHTMLContent("%s%s" % (self.__baseURL, self.__searchURL % urllib.quote(self.__title)))

		# Get all links
		links = self.__fetchMovieLinks()

		# Now retrieve infos
		if links:
			for entry in links:
				data = self.__fetchMovieInfo( url = self.__movieURL + entry[0] )
				# Add DC link (custom field)
				data['darkhorse'] = "%s%s" % (self.__movieURL, entry[0])
				node = self.__domTree.addEntry(data)
				# Print entries on-the-fly
				#self.__domTree.printEntry(node)
		else:
			return None

def halt():
	print "HALT."
	sys.exit(0)

def showUsage():
	print "Usage: %s comic" % sys.argv[0]
	sys.exit(1)

def main():
	if len(sys.argv) < 2:
		showUsage()

	parser = DarkHorseParser()
	parser.run(sys.argv[1])

if __name__ == '__main__':
	main()
