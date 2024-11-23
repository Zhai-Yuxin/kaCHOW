import React, { useEffect, useState, useRef } from 'react';
import Swal from 'sweetalert2'; 
import { AppBar, Toolbar, Typography, IconButton, Box, Grid, Container, Fab, Paper } from '@mui/material';
import { Notifications, MusicNote } from '@mui/icons-material';
import { dynamoDB } from './aws-config'; // Import DynamoDB configuration
import mcqueen from './mcqueen.png';
import topview from './topview.jpg';
import greenTopArc from './greenTopArc.png';
import greenBottomArc from './greenBottomArc.png';
import redTopArc from './redTopArc.png';
import redBottomArc from './redBottomArc.png';


const ProductList = () => {
  const [items, setItems] = useState([]);
  const [loading, setLoading] = useState(true);
  const [latestCommand, setLatestCommand] = useState('No command received yet'); // Default message
  const alertShown = useRef(false); // Ref to track if the alert has been shown
  const [frontObstacle, setFrontObstacle] = useState(false); // false means no obstacle
  const [backObstacle, setBackObstacle] = useState(false);

  // Fetch initial data from DynamoDB
  const fetchItems = async () => {
    const params = { TableName: 'kachow_test' };

    try {
      const data = await dynamoDB.scan(params).promise();
      const fetchedItems = data.Items || [];

      // Sort items by timestamp to find the latest one
      fetchedItems.sort((a, b) => new Date(b.timestamp) - new Date(a.timestamp));

      setItems(fetchedItems);
      if (fetchedItems.length > 0) {
        const command = fetchedItems[0].command; // Get the latest command

        // Update latestCommand based on received command
        if (command === 'cry') {
          setLatestCommand('Baby is crying!'); // Set crying message
          if (!alertShown.current) {
            handleAlert(); // Show alert if command is "cry"
            alertShown.current = true; // Set the ref to true to indicate the alert has been shown
          }
        } else if (command !== 'silence') {
          // Update message for commands other than "none"
          setLatestCommand(`Baby says ${command}`); // Set message for valid commands
          alertShown.current = false; // Reset if the command is not "cry"
        }
      }
    } catch (error) {
      console.error('Error fetching data from DynamoDB:', error);
    } finally {
      setLoading(false);
    }
  };

  // Polling function to fetch items every 1 second
  useEffect(() => {
    fetchItems(); // Initial fetch
    const interval = setInterval(fetchItems, 1000); // Fetch every 1 second

    // Clear interval on component unmount
    return () => clearInterval(interval);
  }, []);

  // Function to show SweetAlert2 notification
  const handleAlert = () => {
    Swal.fire({
      title: 'Oh No!',
      text: 'Baby is crying :(',
      icon: 'warning',
      confirmButtonText: 'OK',
    });
  };

  // Fetch obstacle data from the "kachow_interrupt" table
  const fetchObstacleData = async () => {
    const params = { TableName: 'kachow_interrupt' };

    try {
      const data = await dynamoDB.scan(params).promise();
      const fetchedItems = data.Items || [];

      // Sort by timestamp to get the latest status
      fetchedItems.sort((a, b) => new Date(b.timestamp) - new Date(a.timestamp));

      // Check the latest readings for front and back obstacles
      if (fetchedItems.length > 0) {
        const latestReading = fetchedItems[0];
        const command = latestReading.message;
        console.log(fetchedItems)

        // Update the states based on the latest readings
        if (command === 'front obstacle detected') setFrontObstacle(true);
        if (command === 'front obstacle cleared') setFrontObstacle(false);
        if (command === 'back obstacle detected') setBackObstacle(true);
        if (command === 'back obstacle cleared') setBackObstacle(false);
      }
    } catch (error) {
      console.error('Error fetching obstacle data:', error);
    }
  };

  // Call fetchObstacleData every second
  useEffect(() => {
    fetchObstacleData();
    const interval = setInterval(fetchObstacleData, 1000);
    return () => clearInterval(interval);
  }, []);

  // Function to add an item to another DynamoDB table on Wave button click
  const handleWaveButtonClick = async () => {
    const now = new Date();
    const singaporeTime = new Intl.DateTimeFormat("en-SG", {
      timeZone: "Asia/Singapore",
      year: "numeric",
      month: "2-digit",
      day: "2-digit",
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit"
    }).format(now);

    
    const timestamp = singaporeTime.replace(',', ''); // Format as needed

    const params = {
      TableName: 'reply_esp',
      Item: {
        command: 'wave',
        timestamp,
      },
    };

    try {
      await dynamoDB.put(params).promise(); // Use put() for DocumentClient in SDK v2
      Swal.fire({
        title: 'Success!',
        text: 'Wave action logged!',
        icon: 'success',
        confirmButtonText: 'OK',
      });
    } catch (error) {
      console.error('Error creating Wave log:', error);
      Swal.fire({
        title: 'Error!',
        text: 'Failed to log the Wave action.',
        icon: 'error',
        confirmButtonText: 'OK',
      });
    }
  };

  // Function to send an off to another DynamoDB table on Wave button click
  const handleOffButtonClick = async () => {
    const now = new Date();
    const singaporeTime = new Intl.DateTimeFormat("en-SG", {
      timeZone: "Asia/Singapore",
      year: "numeric",
      month: "2-digit",
      day: "2-digit",
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit"
    }).format(now);

    
    const timestamp = singaporeTime.replace(',', ''); // Format as needed

    const params = {
      TableName: 'reply_esp',
      Item: {
        command: 'off',
        timestamp,
      },
    };

    try {
      await dynamoDB.put(params).promise(); // Use put() for DocumentClient in SDK v2
      Swal.fire({
        title: 'Success!',
        text: 'Vehicle Stop!',
        icon: 'success',
        confirmButtonText: 'OK',
      });
    } catch (error) {
      console.error('Error creating Off log:', error);
      Swal.fire({
        title: 'Error!',
        text: 'Failed to log the off action.',
        icon: 'error',
        confirmButtonText: 'OK',
      });
    }
  };

  // Function to send a music item to another DynamoDB table on Wave button click
  const handleMusicButtonClick = async () => {
    const now = new Date();
    const singaporeTime = new Intl.DateTimeFormat("en-SG", {
      timeZone: "Asia/Singapore",
      year: "numeric",
      month: "2-digit",
      day: "2-digit",
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit"
    }).format(now);

    
    const timestamp = singaporeTime.replace(',', ''); // Format as needed

    const params = {
      TableName: 'reply_esp',
      Item: {
        command: 'music',
        timestamp,
      },
    };

    try {
      await dynamoDB.put(params).promise(); // Use put() for DocumentClient in SDK v2
      Swal.fire({
        title: 'Success!',
        text: 'Music Playing!',
        icon: 'success',
        confirmButtonText: 'OK',
      });
    } catch (error) {
      console.error('Error creating Music log:', error);
      Swal.fire({
        title: 'Error!',
        text: 'Failed to log the Music action.',
        icon: 'error',
        confirmButtonText: 'OK',
      });
    }
  };


  return (
    <Box sx={{ flexGrow: 1 }}>
      {/* AppBar with Alert Button */}
      <AppBar position="static" sx={{ bgcolor: 'black' }}>
        <Toolbar>
          <Typography
            variant="h6"
            component="div"
            sx={{ flexGrow: 1, display: 'flex', alignItems: 'center', gap: 1 }}
          >
            KACHOW
            <img src={mcqueen} alt="McQueen" style={{ width: '50px', height: '50px' }} />
          </Typography>
          <IconButton color="inherit" onClick={handleAlert}>
            <Notifications />
          </IconButton>
        </Toolbar>
      </AppBar>

      {/* Main Content */}
      <Grid container spacing={2} sx={{ padding: 3 }}>
        <Grid item xs={12} md={6}>
          <Grid container spacing={2} direction="column">
            <Grid item>
              <Container
                sx={{
                  bgcolor: 'lightblue',
                  borderRadius: 4,
                  padding: 3,
                  textAlign: 'center',
                }}
              >
                <Typography variant="h5">
                  {latestCommand}
                </Typography>
              </Container>
            </Grid>

            <Grid item>
              <Box sx={{ display: 'flex', gap: 2, justifyContent: 'center', direction:'column' }}>
                <Fab
                  color="primary"
                  variant="extended"
                  sx={{ textTransform: 'none' }}
                  onClick={handleWaveButtonClick} 
                >
                  Wave
                </Fab>
                <Fab 
                  color="error" 
                  variant="extended" 
                  sx={{ textTransform: 'none' }}
                  onClick={handleOffButtonClick}
                >
                  Emergency Stop
                </Fab>
                <Fab 
                  color="outlined" 
                  variant="extended" 
                  sx={{ textTransform: 'none' }}
                  onClick={handleMusicButtonClick}
                > 
                  <MusicNote></MusicNote>
                  Play Music!
                </Fab>
              </Box>
            </Grid>
            <Grid item sx={{ position: 'relative', height: '100%', display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
              <Box sx={{ position: 'absolute', top: 50 }}>
                <img src={frontObstacle === false ? greenTopArc : redTopArc} alt="TopArc" style={{ width: '300px', height: '100px'}} />
              </Box>
              <Box sx={{ position: 'absolute', top: 150 }}>
                <img src={topview} alt="TopView" style={{ width: '200px', height: '400px' }} />
              </Box>
              <Box sx={{ position: 'absolute', top: 570 }}>
                <img src={backObstacle === false ? greenBottomArc : redBottomArc} alt="BottomView" style={{ width: '300px', height: '100px' }} />
              </Box>
            </Grid>


            

          </Grid>
        </Grid>

        <Grid item xs={12} md={6}>
          <Container
            sx={{
              bgcolor: 'lightblue',
              borderRadius: 4,
              padding: 3,
            }}
          >
            <Typography variant="h5" sx={{ mb: 2 }}>
              Actions done by Car
            </Typography>
            {loading ? (
              <Typography>Loading...</Typography>
            ) : items.length > 0 ? (
              <Paper elevation={2} sx={{ padding: 2, height: '700px', overflowY: 'auto' }}>
                <ul>
                  {items.map((item, index) => (
                    <div key={index} style={{ marginBottom: '20px' }}>
                      <li>
                        <strong>Timestamp:</strong> {item.timestamp}
                      </li>
                      <div>
                        <strong>Command:</strong> {item.command}
                      </div>
                      <a href={item.url}>Download audio here</a>
                      <br />
                    </div>
                  ))}
                </ul>
              </Paper>
            ) : (
              <Typography>No items found.</Typography>
            )}
          </Container>
        </Grid>
      </Grid>
    </Box>
  );
};

export default ProductList;
