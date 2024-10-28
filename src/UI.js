import React, { useEffect, useState } from 'react';
import Swal from 'sweetalert2'; 
import { AppBar, Toolbar, Typography, IconButton, Box, Grid, Container, Fab, Paper } from '@mui/material';
import { Notifications } from '@mui/icons-material';
import { dynamoDB } from './aws-config'; // Import DynamoDB configuration


const ProductList = () => {
  const [items, setItems] = useState([]);
  const [loading, setLoading] = useState(true);
  const [latestCommand, setLatestCommand] = useState(null); // Store the latest command

  // Fetch initial data from DynamoDB
  useEffect(() => {
    const fetchItems = async () => {
      const params = { TableName: 'kachow_test' };

      try {
        const data = await dynamoDB.scan(params).promise();
        const fetchedItems = data.Items || [];

        // Sort items by timestamp to find the latest one
        fetchedItems.sort((a, b) => new Date(b.timestamp) - new Date(a.timestamp));

        setItems(fetchedItems);
        if (fetchedItems.length > 0) {
          setLatestCommand(fetchedItems[0].command); // Set the latest command
        }
      } catch (error) {
        console.error('Error fetching data from DynamoDB:', error);
      } finally {
        setLoading(false);
      }
    };

    fetchItems();
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

  // Function to add an item to another DynamoDB table on Wave button click


  const handleWaveButtonClick = async () => {
    const timestamp = new Date().toISOString();
    const params = {
      TableName: 'reply_esp',
      Item: {
        label: 'Wave',
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

  console.log(items);

  return (
    <Box sx={{ flexGrow: 1 }}>
      {/* AppBar with Alert Button */}
      <AppBar position="static" sx={{ bgcolor: 'black' }}>
        <Toolbar>
          <Typography variant="h6" component="div" sx={{ flexGrow: 1 }}>
            KACHOW
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
                  {latestCommand ? (
                    <>
                      Baby said <u>{latestCommand}</u>
                    </>
                  ) : (
                    'No command received yet'
                  )}
                </Typography>
              </Container>
            </Grid>

            <Grid item>
              <Box sx={{ display: 'flex', gap: 2, justifyContent: 'center' }}>
                <Fab
                  color="primary"
                  variant="extended"
                  sx={{ textTransform: 'none' }}
                  onClick={handleWaveButtonClick} // Wave button triggers DynamoDB action
                >
                  Wave
                </Fab>
                <Fab color="error" variant="extended" sx={{ textTransform: 'none' }}>
                  Emergency Switch Off
                </Fab>
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
              height: '100%',
            }}
          >
            <Typography variant="h5" sx={{ mb: 2 }}>
              Actions done by Car
            </Typography>
            {loading ? (
              <Typography>Loading...</Typography>
            ) : items.length > 0 ? (
              <Paper elevation={2} sx={{ padding: 2 }}>
                <ul>
                  {items.map((item, index) => (
                    <div key={index} style={{ marginBottom: '20px' }}>
                      <li>
                        <a style={{ fontWeight: 'bold' }}>Timestamp: </a> {item.timestamp}
                      </li>
                      <div>
                        <a style={{ fontWeight: 'bold' }}>Command: </a> {item.command}
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
